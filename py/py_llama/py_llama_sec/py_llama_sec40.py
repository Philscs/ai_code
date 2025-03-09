import inspect
from types import FrameType
from typing import TypeVar, Generic

T = TypeVar('T')

class BytecodeGenerator(Generic[T]):
    def __init__(self):
        self.code = []

    def generate(self, func: T) -> str:
        """
        Generiert das Bytecode für eine gegebene Funktion.

        Args:
            func (T): Die zu generierende Funktion.

        Returns:
            str: Das generierte Bytecode.
        """
        code = []
        for frame in inspect.getouterframes(frameinfo=func):
            # Verhindern Code-Injection durch die Verwendung von `exec()` und `eval()`
            if isinstance(frame, FrameType) and frame.filename == '<string>':
                continue
            self.code.append(f"def {frame.name}(self):")
            # Generiere ein Beispiel für eine Funktion, die Ressourcen limits durchsetzt
            code.append("    pass")

        return ''.join(code)

class SandboxExecutor:
    def __init__(self):
        self.executed_code = {}

    def execute(self, bytecode: str) -> str:
        """
        Durchführt das Bytecode in einem Sandbox-UMgebung.

        Args:
            bytecode (str): Das zu durchführende Bytecode.

        Returns:
            str: Die Ausgabe des durchgeführten Code.
        """
        # Verhindern Code-Injection durch die Verwendung von `exec()` und `eval()`
        if 'exec' in bytecode or 'eval' in bytecode:
            raise ValueError("Code-Injection verboten")

        sandboxed_code = self.sandbox(bytecode)
        return f"{''.join(sandboxed_code)}"

    def sandbox(self, bytecode: str) -> list[str]:
        """
        Durchführt das Bytecode in einem Sandbox-UMgebung.

        Args:
            bytecode (str): Das zu durchführende Bytecode.

        Returns:
            list[str]: Die gegebenen Ressourcenlimits.
        """
        # Beispiel für eine Funktion, die Ressourcen limits durchsetzt
        if 'pass' in bytecode:
            return ['print("Ressourcenlimit erreicht")']
        return []

if __name__ == "__main__":
    def example_func():
        pass

    generator = BytecodeGenerator()
    bytecode = generator.generate(example_func)
    print(bytecode)

    executor = SandboxExecutor()
    result = executor.execute(bytecode)
    print(result)