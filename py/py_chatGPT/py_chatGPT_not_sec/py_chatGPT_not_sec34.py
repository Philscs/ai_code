import os
import ast
import networkx as nx
import matplotlib.pyplot as plt

class DependencyAnalyzer:
    def __init__(self, root_dir):
        self.root_dir = root_dir
        self.graph = nx.DiGraph()

    def analyze(self):
        """Analysiere alle Python-Dateien im Verzeichnis."""
        for dirpath, _, filenames in os.walk(self.root_dir):
            for file in filenames:
                if file.endswith(".py"):
                    filepath = os.path.join(dirpath, file)
                    self._analyze_file(filepath)

    def _analyze_file(self, filepath):
        """Analysiere eine einzelne Python-Datei."""
        with open(filepath, "r", encoding="utf-8") as file:
            try:
                tree = ast.parse(file.read(), filename=filepath)
                self._process_ast(tree, filepath)
            except SyntaxError as e:
                print(f"Syntaxfehler in Datei {filepath}: {e}")

    def _process_ast(self, tree, filepath):
        """Extrahiere Abhängigkeiten aus dem AST der Datei."""
        for node in ast.walk(tree):
            if isinstance(node, ast.Import):
                for alias in node.names:
                    self.graph.add_edge(filepath, alias.name)
            elif isinstance(node, ast.ImportFrom):
                if node.module:
                    self.graph.add_edge(filepath, node.module)

    def visualize(self):
        """Visualisiere den Abhängigkeitsgraphen."""
        pos = nx.spring_layout(self.graph)
        plt.figure(figsize=(12, 8))
        nx.draw(self.graph, pos, with_labels=True, node_size=3000, font_size=10, node_color="lightblue", edge_color="gray")
        plt.title("Abhängigkeitsgraph")
        plt.show()

    def impact_analysis(self, target_file):
        """Führe eine Impact-Analyse durch, um abhängige Dateien zu finden."""
        if target_file not in self.graph:
            print(f"Datei {target_file} ist nicht im Graphen vorhanden.")
            return []

        impacted_files = nx.descendants(self.graph, target_file)
        print(f"Folgende Dateien sind von {target_file} abhängig:")
        for file in impacted_files:
            print(f"- {file}")
        return impacted_files

if __name__ == "__main__":
    # Verzeichnis mit Python-Dateien angeben
    root_directory = "./mein_python_projekt"

    analyzer = DependencyAnalyzer(root_directory)
    analyzer.analyze()

    # Visualisierung des Abhängigkeitsgraphen
    analyzer.visualize()

    # Impact-Analyse für eine spezifische Datei
    target = "./mein_python_projekt/beispiel.py"
    analyzer.impact_analysis(target)
