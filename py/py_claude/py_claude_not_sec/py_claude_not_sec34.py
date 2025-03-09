import os
import ast
import networkx as nx
import matplotlib.pyplot as plt
from typing import Dict, Set, List, Tuple

class DependencyAnalyzer:
    def __init__(self, root_dir: str):
        """
        Initialisiert den Dependency Analyzer.
        
        Args:
            root_dir (str): Wurzelverzeichnis des zu analysierenden Projekts
        """
        self.root_dir = root_dir
        self.dependency_graph = nx.DiGraph()
        self.module_imports: Dict[str, Set[str]] = {}
        
    def parse_file(self, file_path: str) -> Set[str]:
        """
        Analysiert eine Python-Datei und extrahiert die Importabhängigkeiten.
        
        Args:
            file_path (str): Pfad zur Python-Datei
            
        Returns:
            Set[str]: Menge der gefundenen Imports
        """
        imports = set()
        try:
            with open(file_path, 'r', encoding='utf-8') as file:
                tree = ast.parse(file.read())
                
            for node in ast.walk(tree):
                if isinstance(node, ast.Import):
                    for name in node.names:
                        imports.add(name.name)
                elif isinstance(node, ast.ImportFrom):
                    module = node.module if node.module else ''
                    for name in node.names:
                        imports.add(f"{module}.{name.name}")
        except Exception as e:
            print(f"Fehler beim Parsen von {file_path}: {e}")
            
        return imports
    
    def analyze_project(self):
        """Analysiert das gesamte Projekt und erstellt den Abhängigkeitsgraphen."""
        for root, _, files in os.walk(self.root_dir):
            for file in files:
                if file.endswith('.py'):
                    file_path = os.path.join(root, file)
                    relative_path = os.path.relpath(file_path, self.root_dir)
                    module_name = os.path.splitext(relative_path)[0].replace(os.sep, '.')
                    
                    imports = self.parse_file(file_path)
                    self.module_imports[module_name] = imports
                    
                    # Füge Knoten und Kanten zum Graphen hinzu
                    self.dependency_graph.add_node(module_name)
                    for imp in imports:
                        self.dependency_graph.add_edge(module_name, imp)
    
    def visualize_dependencies(self, output_file: str = 'dependencies.png'):
        """
        Visualisiert den Abhängigkeitsgraphen.
        
        Args:
            output_file (str): Pfad für die Ausgabedatei
        """
        plt.figure(figsize=(12, 8))
        pos = nx.spring_layout(self.dependency_graph)
        nx.draw(self.dependency_graph, pos, 
                with_labels=True, 
                node_color='lightblue',
                node_size=2000,
                font_size=8,
                font_weight='bold',
                arrows=True,
                edge_color='gray')
        plt.savefig(output_file)
        plt.close()
    
    def analyze_impact(self, module_name: str) -> Tuple[Set[str], Set[str]]:
        """
        Führt eine Impact-Analyse für ein bestimmtes Modul durch.
        
        Args:
            module_name (str): Name des zu analysierenden Moduls
            
        Returns:
            Tuple[Set[str], Set[str]]: (Abhängige Module, Module von denen abhängig)
        """
        # Finde alle Module, die von diesem Modul abhängig sind
        dependent_modules = set()
        for module, imports in self.module_imports.items():
            if module_name in imports:
                dependent_modules.add(module)
        
        # Finde alle Module, von denen dieses Modul abhängig ist
        dependencies = self.module_imports.get(module_name, set())
        
        return dependent_modules, dependencies
    
    def generate_impact_report(self, module_name: str) -> str:
        """
        Generiert einen detaillierten Impact-Bericht für ein Modul.
        
        Args:
            module_name (str): Name des zu analysierenden Moduls
            
        Returns:
            str: Formatierter Impact-Bericht
        """
        dependent_modules, dependencies = self.analyze_impact(module_name)
        
        report = [
            f"Impact-Analyse für Modul: {module_name}\n",
            "\nModule, die von Änderungen betroffen wären:",
            "----------------------------------------"
        ]
        
        if dependent_modules:
            for module in sorted(dependent_modules):
                report.append(f"- {module}")
        else:
            report.append("Keine abhängigen Module gefunden.")
            
        report.extend([
            "\nModule, von denen dieses Modul abhängt:",
            "----------------------------------------"
        ])
        
        if dependencies:
            for module in sorted(dependencies):
                report.append(f"- {module}")
        else:
            report.append("Keine Abhängigkeiten gefunden.")
            
        return "\n".join(report)

def main():
    """Beispielverwendung des DependencyAnalyzers"""
    # Projektverzeichnis angeben
    project_dir = "path/to/your/project"
    
    # Analyzer initialisieren und Projekt analysieren
    analyzer = DependencyAnalyzer(project_dir)
    analyzer.analyze_project()
    
    # Abhängigkeiten visualisieren
    analyzer.visualize_dependencies()
    
    # Impact-Analyse für ein bestimmtes Modul durchführen
    module_name = "your.module.name"
    report = analyzer.generate_impact_report(module_name)
    print(report)

if __name__ == "__main__":
    main()