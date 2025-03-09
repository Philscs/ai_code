import json
from abc import ABC, abstractmethod

# Design Patterns Implementierungen
class SingletonGenerator:
    def generate(self, class_name):
        return f"""
class {class_name}:
    _instance = None

    def __new__(cls, *args, **kwargs):
        if not cls._instance:
            cls._instance = super({class_name}, cls).__new__(cls, *args, **kwargs)
        return cls._instance
"""

class FactoryGenerator:
    def generate(self, class_name):
        return f"""
class {class_name}Factory:
    @staticmethod
    def create_instance():
        return {class_name}()

class {class_name}:
    pass
"""

# Abstrakte Generator-Klasse
class UMLCodeGenerator(ABC):
    @abstractmethod
    def generate_code(self, uml_data):
        pass

# Python-Code-Generator
class PythonCodeGenerator(UMLCodeGenerator):
    def __init__(self):
        self.pattern_generators = {
            "Singleton": SingletonGenerator(),
            "Factory": FactoryGenerator()
        }

    def generate_code(self, uml_data):
        classes = uml_data.get("classes", [])
        generated_code = ""

        for cls in classes:
            class_name = cls["name"]
            pattern = cls.get("pattern")

            if pattern in self.pattern_generators:
                generated_code += self.pattern_generators[pattern].generate(class_name)
            else:
                generated_code += f"class {class_name}:
    pass\n\n"

        return generated_code

# UML-Parser
class UMLParser:
    def parse(self, uml_json):
        try:
            return json.loads(uml_json)
        except json.JSONDecodeError as e:
            raise ValueError("Invalid UML JSON data") from e

# Hauptprogramm
def main():
    # Beispiel-UML-Diagramm in JSON
    uml_json = """
    {
        "classes": [
            {"name": "User", "pattern": "Singleton"},
            {"name": "Product", "pattern": "Factory"},
            {"name": "Order"}
        ]
    }
    """

    parser = UMLParser()
    uml_data = parser.parse(uml_json)

    generator = PythonCodeGenerator()
    generated_code = generator.generate_code(uml_data)

    print("Generated Code:\n")
    print(generated_code)

if __name__ == "__main__":
    main()
