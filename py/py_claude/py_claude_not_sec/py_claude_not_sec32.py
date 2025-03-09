import ast
from typing import List, Dict, Any
from dataclasses import dataclass
from enum import Enum
import statistics

class CodeSmellType(Enum):
    LONG_METHOD = "Long Method"
    LARGE_CLASS = "Large Class"
    LONG_PARAMETER_LIST = "Long Parameter List"
    DUPLICATE_CODE = "Duplicate Code"
    COMPLEX_CONDITIONAL = "Complex Conditional"
    DATA_CLASS = "Data Class"

@dataclass
class CodeSmell:
    type: CodeSmellType
    location: str
    description: str
    refactoring_suggestion: str
    severity: int  # 1-5, wobei 5 am kritischsten ist

class CodeSmellDetector:
    def __init__(self, source_code: str):
        self.source_code = source_code
        self.tree = ast.parse(source_code)
        self.smells: List[CodeSmell] = []
        
        # Konfigurierbare Schwellenwerte
        self.config = {
            "max_method_length": 15,  # Zeilen
            "max_class_length": 200,  # Zeilen
            "max_parameters": 4,
            "max_complexity": 10,
            "min_duplicate_lines": 6
        }

    def analyze(self) -> List[CodeSmell]:
        """Führt alle Code-Smell-Analysen durch."""
        self.detect_long_methods()
        self.detect_large_classes()
        self.detect_long_parameter_lists()
        self.detect_complex_conditionals()
        self.detect_data_classes()
        self.detect_duplicate_code()
        return self.smells

    def detect_long_methods(self):
        """Erkennt Methoden, die zu lang sind."""
        for node in ast.walk(self.tree):
            if isinstance(node, ast.FunctionDef):
                method_length = len(node.body)
                if method_length > self.config["max_method_length"]:
                    self.smells.append(CodeSmell(
                        type=CodeSmellType.LONG_METHOD,
                        location=f"Methode: {node.name}",
                        description=f"Methode ist {method_length} Zeilen lang (Maximum: {self.config['max_method_length']})",
                        refactoring_suggestion="Extrahiere logische Blöcke in separate Methoden. "
                                            "Verwende das Extract Method Pattern.",
                        severity=min(5, method_length // self.config["max_method_length"])
                    ))

    def detect_large_classes(self):
        """Erkennt zu große Klassen."""
        for node in ast.walk(self.tree):
            if isinstance(node, ast.ClassDef):
                class_length = sum(len(method.body) for method in node.body 
                                 if isinstance(method, ast.FunctionDef))
                if class_length > self.config["max_class_length"]:
                    self.smells.append(CodeSmell(
                        type=CodeSmellType.LARGE_CLASS,
                        location=f"Klasse: {node.name}",
                        description=f"Klasse hat {class_length} Zeilen Code",
                        refactoring_suggestion="Teile die Klasse in kleinere, spezialisierte Klassen auf. "
                                            "Verwende das Single Responsibility Principle.",
                        severity=min(5, class_length // self.config["max_class_length"])
                    ))

    def detect_long_parameter_lists(self):
        """Erkennt Methoden mit zu vielen Parametern."""
        for node in ast.walk(self.tree):
            if isinstance(node, ast.FunctionDef):
                param_count = len([arg for arg in node.args.args 
                                 if arg.arg != 'self'])
                if param_count > self.config["max_parameters"]:
                    self.smells.append(CodeSmell(
                        type=CodeSmellType.LONG_PARAMETER_LIST,
                        location=f"Methode: {node.name}",
                        description=f"Methode hat {param_count} Parameter",
                        refactoring_suggestion="Gruppiere zusammenhängende Parameter in eigene Objekte. "
                                            "Verwende das Parameter Object Pattern.",
                        severity=min(5, param_count - self.config["max_parameters"] + 1)
                    ))

    def detect_complex_conditionals(self):
        """Erkennt komplexe Bedingungen."""
        class ComplexityVisitor(ast.NodeVisitor):
            def __init__(self):
                self.complexity = 0

            def visit_If(self, node):
                self.complexity += 1
                self.generic_visit(node)

            def visit_BoolOp(self, node):
                self.complexity += len(node.values) - 1
                self.generic_visit(node)

        for node in ast.walk(self.tree):
            if isinstance(node, ast.FunctionDef):
                visitor = ComplexityVisitor()
                visitor.visit(node)
                if visitor.complexity > self.config["max_complexity"]:
                    self.smells.append(CodeSmell(
                        type=CodeSmellType.COMPLEX_CONDITIONAL,
                        location=f"Methode: {node.name}",
                        description=f"Komplexität der Bedingungen: {visitor.complexity}",
                        refactoring_suggestion="Extrahiere Bedingungen in separate Methoden. "
                                            "Verwende das Strategy Pattern für verschiedene Fälle.",
                        severity=min(5, visitor.complexity - self.config["max_complexity"] + 1)
                    ))

    def detect_data_classes(self):
        """Erkennt Data Classes (Klassen mit hauptsächlich Gettern/Settern)."""
        for node in ast.walk(self.tree):
            if isinstance(node, ast.ClassDef):
                methods = [m for m in node.body if isinstance(m, ast.FunctionDef)]
                getter_setter_count = sum(1 for m in methods 
                                        if m.name.startswith(('get_', 'set_')))
                if getter_setter_count > len(methods) * 0.8 and len(methods) > 3:
                    self.smells.append(CodeSmell(
                        type=CodeSmellType.DATA_CLASS,
                        location=f"Klasse: {node.name}",
                        description=f"{getter_setter_count} von {len(methods)} Methoden sind Getter/Setter",
                        refactoring_suggestion="Verschiebe Verhalten in die Datenklasse. "
                                            "Verwende Tell, Don't Ask Principle.",
                        severity=3
                    ))

    def detect_duplicate_code(self):
        """Erkennt duplizierte Code-Blöcke (vereinfachte Implementierung)."""
        def get_code_blocks(node: ast.AST, min_lines: int) -> List[str]:
            blocks = []
            for child in ast.walk(node):
                if isinstance(child, ast.FunctionDef):
                    code_lines = [line.strip() for line in 
                                ast.unparse(child).split('\n')]
                    for i in range(len(code_lines) - min_lines + 1):
                        block = '\n'.join(code_lines[i:i + min_lines])
                        if block.strip():
                            blocks.append(block)
            return blocks

        blocks = get_code_blocks(self.tree, self.config["min_duplicate_lines"])
        seen_blocks = {}
        
        for block in blocks:
            if block in seen_blocks:
                seen_blocks[block] += 1
            else:
                seen_blocks[block] = 1

        for block, count in seen_blocks.items():
            if count > 1 and len(block.split('\n')) >= self.config["min_duplicate_lines"]:
                self.smells.append(CodeSmell(
                    type=CodeSmellType.DUPLICATE_CODE,
                    location="Mehrere Stellen",
                    description=f"Code-Block erscheint {count} Mal",
                    refactoring_suggestion="Extrahiere duplizierten Code in eine gemeinsame Methode. "
                                        "Verwende Extract Method und Template Method Pattern.",
                    severity=min(5, count)
                ))

class RefactoringReporter:
    """Generiert formatierte Berichte über gefundene Code-Smells."""
    
    def __init__(self, smells: List[CodeSmell]):
        self.smells = smells

    def generate_report(self) -> str:
        """Erstellt einen formatierten Bericht über alle gefundenen Code-Smells."""
        if not self.smells:
            return "Keine Code-Smells gefunden."

        report = ["=== Code-Smell-Analyse Bericht ===\n"]
        
        # Gruppiere nach Smell-Typ
        smells_by_type: Dict[CodeSmellType, List[CodeSmell]] = {}
        for smell in self.smells:
            if smell.type not in smells_by_type:
                smells_by_type[smell.type] = []
            smells_by_type[smell.type].append(smell)

        # Erstelle Bericht für jeden Typ
        for smell_type, smells in smells_by_type.items():
            report.append(f"\n## {smell_type.value}")
            report.append(f"Gefundene Instanzen: {len(smells)}")
            
            # Sortiere nach Schweregrad
            smells.sort(key=lambda x: x.severity, reverse=True)
            
            for smell in smells:
                report.append(f"\n- Stelle: {smell.location}")
                report.append(f"  Schweregrad: {'*' * smell.severity}")
                report.append(f"  Problem: {smell.description}")
                report.append(f"  Vorschlag: {smell.refactoring_suggestion}")

        return "\n".join(report)

    def get_statistics(self) -> Dict[str, Any]:
        """Berechnet Statistiken über die gefundenen Code-Smells."""
        if not self.smells:
            return {"total_smells": 0}

        stats = {
            "total_smells": len(self.smells),
            "average_severity": statistics.mean(smell.severity for smell in self.smells),
            "smells_by_type": {type.value: 0 for type in CodeSmellType},
            "max_severity": max(smell.severity for smell in self.smells),
            "min_severity": min(smell.severity for smell in self.smells)
        }

        for smell in self.smells:
            stats["smells_by_type"][smell.type.value] += 1

        return stats

# Beispielnutzung:
if __name__ == "__main__":
    # Beispiel-Code zum Analysieren
    sample_code = """
class UserManager:
    def __init__(self):
        self.users = []

    def add_user(self, name, email, age, address, phone, title, department, 
                 salary, start_date, manager):
        user = {
            'name': name,
            'email': email,
            'age': age,
            'address': address,
            'phone': phone,
            'title': title,
            'department': department,
            'salary': salary,
            'start_date': start_date,
            'manager': manager
        }
        self.users.append(user)

    def get_user_by_email(self, email):
        for user in self.users:
            if user['email'] == email:
                return user
        return None

    def update_user(self, email, **kwargs):
        user = self.get_user_by_email(email)
        if user:
            for key, value in kwargs.items():
                user[key] = value
    """

    # Code-Smell-Analyse durchführen
    detector = CodeSmellDetector(sample_code)
    smells = detector.analyze()

    # Bericht generieren
    reporter = RefactoringReporter(smells)
    print(reporter.generate_report())
    print("\nStatistiken:", reporter.get_statistics())