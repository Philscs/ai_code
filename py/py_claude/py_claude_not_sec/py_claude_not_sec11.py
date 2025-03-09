import ast
import inspect
import importlib
import os
from typing import Dict, List, Optional, Set
from dataclasses import dataclass
import markdown
import yaml

@dataclass
class CodeEntity:
    """Repräsentiert eine Code-Einheit (Klasse, Funktion, etc.)."""
    name: str
    docstring: str
    type: str
    dependencies: Set[str]
    source: str
    
class DependencyAnalyzer:
    """Analysiert Code-Abhängigkeiten durch AST-Parsing."""
    
    def __init__(self):
        self.dependencies = set()
        
    def visit(self, node: ast.AST) -> None:
        """Durchläuft den AST und sammelt Abhängigkeiten."""
        if isinstance(node, ast.Import):
            for name in node.names:
                self.dependencies.add(name.name)
        elif isinstance(node, ast.ImportFrom):
            if node.module:
                self.dependencies.add(node.module)
        
        for child in ast.iter_child_nodes(node):
            self.visit(child)
            
    def get_dependencies(self, source: str) -> Set[str]:
        """Extrahiert Abhängigkeiten aus dem Quellcode."""
        tree = ast.parse(source)
        self.dependencies.clear()
        self.visit(tree)
        return self.dependencies

class DocstringParser:
    """Parst und verarbeitet Python Docstrings."""
    
    @staticmethod
    def parse_docstring(obj) -> str:
        """Extrahiert und bereinigt den Docstring eines Objekts."""
        doc = inspect.getdoc(obj)
        return doc if doc else ""

class DocumentationGenerator:
    """Hauptklasse für die Dokumentationsgenerierung."""
    
    def __init__(self):
        self.dependency_analyzer = DependencyAnalyzer()
        self.entities: Dict[str, CodeEntity] = {}
        
    def analyze_module(self, module_path: str) -> None:
        """Analysiert ein Python-Modul und extrahiert Code-Entities."""
        spec = importlib.util.spec_from_file_location("module", module_path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        
        for name, obj in inspect.getmembers(module):
            if inspect.isclass(obj) or inspect.isfunction(obj):
                source = inspect.getsource(obj)
                docstring = DocstringParser.parse_docstring(obj)
                dependencies = self.dependency_analyzer.get_dependencies(source)
                
                entity_type = "class" if inspect.isclass(obj) else "function"
                self.entities[name] = CodeEntity(
                    name=name,
                    docstring=docstring,
                    type=entity_type,
                    dependencies=dependencies,
                    source=source
                )
    
    def generate_markdown(self, output_path: str) -> None:
        """Generiert Markdown-Dokumentation."""
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write("# Automatisch generierte Dokumentation\n\n")
            
            # Klassen dokumentieren
            f.write("## Klassen\n\n")
            for entity in self.entities.values():
                if entity.type == "class":
                    f.write(f"### {entity.name}\n\n")
                    f.write(f"{entity.docstring}\n\n")
                    f.write("#### Abhängigkeiten\n\n")
                    for dep in entity.dependencies:
                        f.write(f"- {dep}\n")
                    f.write("\n")
            
            # Funktionen dokumentieren
            f.write("## Funktionen\n\n")
            for entity in self.entities.values():
                if entity.type == "function":
                    f.write(f"### {entity.name}\n\n")
                    f.write(f"{entity.docstring}\n\n")
                    f.write("#### Abhängigkeiten\n\n")
                    for dep in entity.dependencies:
                        f.write(f"- {dep}\n")
                    f.write("\n")
    
    def generate_html(self, output_path: str) -> None:
        """Generiert HTML-Dokumentation aus Markdown."""
        md_content = ""
        
        md_content += "# Automatisch generierte Dokumentation\n\n"
        
        for entity in self.entities.values():
            md_content += f"## {entity.name}\n\n"
            md_content += f"**Typ:** {entity.type}\n\n"
            md_content += f"{entity.docstring}\n\n"
            md_content += "### Abhängigkeiten\n\n"
            for dep in entity.dependencies:
                md_content += f"- {dep}\n"
            md_content += "\n"
        
        html_content = markdown.markdown(md_content)
        
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(f"""
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Dokumentation</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 40px; }}
        code {{ background-color: #f5f5f5; padding: 2px 4px; }}
    </style>
</head>
<body>
{html_content}
</body>
</html>
""")
    
    def generate_yaml(self, output_path: str) -> None:
        """Generiert YAML-Dokumentation."""
        doc_data = {
            'entities': [
                {
                    'name': entity.name,
                    'type': entity.type,
                    'docstring': entity.docstring,
                    'dependencies': list(entity.dependencies)
                }
                for entity in self.entities.values()
            ]
        }
        
        with open(output_path, 'w', encoding='utf-8') as f:
            yaml.dump(doc_data, f, allow_unicode=True, sort_keys=False)

def main():
    """Beispiel für die Verwendung des Dokumentationsgenerators."""
    generator = DocumentationGenerator()
    
    # Modul analysieren
    generator.analyze_module("path/to/your/module.py")
    
    # Dokumentation in verschiedenen Formaten generieren
    generator.generate_markdown("docs/documentation.md")
    generator.generate_html("docs/documentation.html")
    generator.generate_yaml("docs/documentation.yaml")

if __name__ == "__main__":
    main()