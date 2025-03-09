import ast
from typing import List

class CodeAnalyzer:
    def __init__(self, code: str):
        self.code = code

    def analyze(self) -> List[ast.Node]:
        # Analyse des Codes
        tree = ast.parse(self.code)
        return list(tree.body)

    def identify_performance_bottlenecks(self, nodes: List[ast.Node]) -> None:
        # Identifizierung von Performance-Bottlenecks
        bottlenecks = []
        for node in nodes:
            if isinstance(node, ast.For) or isinstance(node, ast.While):
                bottlenecks.append((node.lineno, node.col_offset))
        print("Performance-Bottlenecks gefunden:")
        for bottleneck in bottlenecks:
            print(f"Zeile {bottleneck[0]} Kolonne {bottleneck[1]}")

    def suggest_improvements(self, nodes: List[ast.Node]) -> None:
        # Vorschläge für Verbesserungen
        improvements = []
        for node in nodes:
            if isinstance(node, ast.For):
                improvements.append(f"Verwenden Sie eine generator-basierte Schleife")
            elif isinstance(node, ast.While):
                improvements.append("Überprüfen Sie die Bedingung des While-Schleifs")
        print("\nVorschläge für Verbesserungen:")
        for improvement in improvements:
            print(improvement)

    def optimize_code(self, nodes: List[ast.Node]) -> None:
        # Automatisierte Code-Optimierung
        optimized_code = ""
        for node in nodes:
            if isinstance(node, ast.For):
                optimized_code += "# Verwenden Sie eine generator-basierte Schleife\n"
            elif isinstance(node, ast.While):
                optimized_code += "# Überprüfen Sie die Bedingung des While-Schleifs\n"
        print("\nOptimierter Code:")
        print(optimized_code)
