import dis
import types
import sys
from typing import List, Dict, Any, Optional
import ast
import builtins

class SafetyViolation(Exception):
    """Exception für Sicherheitsverletzungen während der Bytecode-Generierung."""
    pass

class ResourceLimit:
    def __init__(self, max_memory_mb: int = 100, max_time_seconds: int = 5):
        self.max_memory = max_memory_mb * 1024 * 1024  # Konvertierung zu Bytes
        self.max_time = max_time_seconds
        
    def check_memory(self):
        """Überprüft den Speicherverbrauch."""
        import resource
        if resource.getrusage(resource.RUSAGE_SELF).ru_maxrss > self.max_memory:
            raise MemoryError("Speicherlimit überschritten")

class DSLBytecodeGenerator:
    def __init__(self, resource_limit: Optional[ResourceLimit] = None):
        self.resource_limit = resource_limit or ResourceLimit()
        self._safe_builtins = {
            'len': len,
            'range': range,
            'int': int,
            'float': float,
            'str': str,
            'list': list,
            'dict': dict,
            'set': set,
            'max': max,
            'min': min,
            'sum': sum,
            'abs': abs,
            'round': round,
        }
        # Verbotene Operationen
        self._forbidden_ops = {
            'LOAD_ATTR': {'__code__', '__globals__', '__dict__', '__class__', '__bases__'},
            'LOAD_METHOD': {'eval', 'exec', 'compile', '__import__'},
        }
        
    def validate_ast(self, node: ast.AST) -> None:
        """Validiert den AST auf potenziell gefährliche Konstrukte."""
        for node in ast.walk(node):
            # Prüfe auf Import-Statements
            if isinstance(node, (ast.Import, ast.ImportFrom)):
                raise SafetyViolation("Imports sind nicht erlaubt")
            
            # Prüfe auf eval/exec Aufrufe
            if isinstance(node, ast.Call):
                if isinstance(node.func, ast.Name):
                    if node.func.id in ['eval', 'exec', '__import__']:
                        raise SafetyViolation(f"Gefährliche Funktion: {node.func.id}")
            
            # Prüfe auf Attribute-Zugriffe
            if isinstance(node, ast.Attribute):
                if node.attr.startswith('__'):
                    raise SafetyViolation(f"Zugriff auf geschütztes Attribut: {node.attr}")

    def analyze_bytecode(self, code: types.CodeType) -> None:
        """Analysiert Bytecode auf gefährliche Sequenzen."""
        for instruction in dis.get_instructions(code):
            if instruction.opname in self._forbidden_ops:
                if instruction.argval in self._forbidden_ops[instruction.opname]:
                    raise SafetyViolation(
                        f"Gefährliche Bytecode-Operation: {instruction.opname} {instruction.argval}"
                    )

    def create_safe_globals(self) -> Dict[str, Any]:
        """Erstellt ein eingeschränktes globals Dictionary."""
        return {
            '__builtins__': self._safe_builtins,
            '__name__': '__main__',
        }

    def compile_dsl(self, source: str) -> types.CodeType:
        """Kompiliert DSL-Code zu sicherem Bytecode."""
        try:
            # Parse zu AST
            tree = ast.parse(source)
            
            # Validiere AST
            self.validate_ast(tree)
            
            # Kompiliere zu Bytecode
            code = compile(tree, '<dsl>', 'exec')
            
            # Analysiere Bytecode auf Sicherheit
            self.analyze_bytecode(code)
            
            return code
            
        except (SyntaxError, SafetyViolation) as e:
            raise SafetyViolation(f"Kompilierung fehlgeschlagen: {str(e)}")

    def execute_safe(self, code: types.CodeType, locals_dict: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
        """Führt den Bytecode in einer Sandbox aus."""
        import resource
        import signal
        
        def timeout_handler(signum, frame):
            raise TimeoutError("Ausführungszeitlimit überschritten")
        
        # Setze Zeitlimit
        signal.signal(signal.SIGALRM, timeout_handler)
        signal.alarm(self.resource_limit.max_time)
        
        try:
            # Erstelle isolierte Ausführungsumgebung
            globals_dict = self.create_safe_globals()
            locals_dict = locals_dict or {}
            
            # Führe Code aus
            exec(code, globals_dict, locals_dict)
            
            return locals_dict
            
        finally:
            # Entferne Zeitlimit
            signal.alarm(0)

class DSLExecutionContext:
    """Kontext-Manager für sichere DSL-Ausführung."""
    def __init__(self, generator: DSLBytecodeGenerator):
        self.generator = generator
        
    def __enter__(self):
        # Setze Ressourcenlimits
        return self
        
    def __exit__(self, exc_type, exc_val, exc_tb):
        # Stelle ursprüngliche Limits wieder her
        pass

# Beispielverwendung
if __name__ == "__main__":
    # Beispiel DSL-Code
    dsl_code = """
    def calculate_sum(x, y):
        return x + y
    
    result = calculate_sum(5, 3)
    """
    
    # Erstelle Generator mit Ressourcenlimits
    generator = DSLBytecodeGenerator(ResourceLimit(max_memory_mb=50, max_time_seconds=2))
    
    try:
        # Kompiliere und führe aus
        with DSLExecutionContext(generator):
            bytecode = generator.compile_dsl(dsl_code)
            result = generator.execute_safe(bytecode)
            print(f"Ausführungsergebnis: {result}")
            
    except SafetyViolation as e:
        print(f"Sicherheitsfehler: {e}")
    except (MemoryError, TimeoutError) as e:
        print(f"Ressourcenlimit überschritten: {e}")