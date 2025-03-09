

@dataclass
class PerformanceMetrics:
    execution_time: float
    memory_usage: float
    function_calls: int
    complexity_score: int


class CodeOptimizer:
    def __init__(self):
        self.profiler = cProfile.Profile()

    def analyze_code(self, code: str) -> Dict[str, Any]:
        """
        Analysiert den Code auf verschiedene Metriken und identifiziert Bottlenecks
        """
        tree = ast.parse(code)

        # Komplexitätsanalyse
        complexity = self._calculate_complexity(tree)

        # Performance-Messung
        exec_time = self._measure_execution_time(code)

        # Profiling-Daten sammeln
        profiling_stats = self._collect_profiling_data(code)

        return {
            "complexity": complexity,
            "execution_time": exec_time,
            "profiling": profiling_stats,
            "optimization_suggestions": self._generate_suggestions(tree, complexity, exec_time)
        }

    def _calculate_complexity(self, tree: ast.AST) -> int:
        """
        Berechnet die zyklomatische Komplexität des Codes
        """
        complexity = 1  # Basis-Komplexität

        for node in ast.walk(tree):
            if isinstance(node, (ast.If, ast.While, ast.For, ast.Assert,
                                 ast.Try, ast.ExceptHandler)):
                complexity += 1
            elif isinstance(node, ast.BoolOp):
                complexity += len(node.values) - 1

        return complexity

    @profile
    def _measure_execution_time(self, code: str) -> float:
        """
        Misst die Ausführungszeit des Codes
        """
        start_time = time.time()
        exec(code)
        return time.time() - start_time

    def _collect_profiling_data(self, code: str) -> Dict[str, Any]:
        """
        Sammelt detaillierte Profiling-Daten
        """
        self.profiler.enable()
        exec(code)
        self.profiler.disable()

        stats = pstats.Stats(self.profiler)

        function_calls = {}
        for func, (cc, nc, tt, ct, callers) in stats.stats.items():
            function_calls[func[2]] = {
                "calls": cc,
                "time": tt,
                "cumulative_time": ct
            }

        return function_calls

    def _generate_suggestions(self, tree: ast.AST, complexity: int,
                              exec_time: float) -> List[str]:
        """
        Generiert Optimierungsvorschläge basierend auf der Analyse
        """
        suggestions = []

        # Komplexitätsbasierte Vorschläge
        if complexity > 10:
            suggestions.append("Hohe Komplexität: Erwägen Sie die Aufteilung in kleinere Funktionen")

        # Performance-basierte Vorschläge
        if exec_time > 1.0:
            suggestions.append("Lange Ausführungszeit: Prüfen Sie Schleifen und IO-Operationen")

        # Code-Pattern Analyse
        for node in ast.walk(tree):
            if isinstance(node, ast.ListComp) and complexity > 5:
                suggestions.append("Liste-Comprehension in komplexem Code: "
                                    "Prüfen Sie auf bessere Lesbarkeit mit normaler Schleife")

            if isinstance(node, ast.For):
                suggestions.append("Prüfen Sie, ob die Schleife durch vectorisierte Operationen "
                                    "(z.B. numpy) ersetzt werden kann")

        return suggestions


class CodeOptimizationReport:
    def __init__(self, analysis_results: Dict[str, Any]):
        self.results = analysis_results

    def generate_report(self) -> str:
        """
        Generiert einen formatierten Bericht der Optimierungsanalyse
        """
        report = []
        report.append("=== Code-Optimierungs-Bericht ===\n")

        # Komplexität
        report.append(f"Komplexität: {self.results['complexity']}")
        report.append(f"Ausführungszeit: {self.results['execution_time']:.4f} Sekunden\n")

        # Profiling-Zusammenfassung
        report.append("Profiling-Details:")
        for func, stats in self.results['profiling'].items():
            report.append(f"  {func}:")
            report.append(f"    - Aufrufe: {stats['calls']}")
            report.append(f"    - Zeit: {stats['time']:.4f} Sekunden")
            report.append(f"    - Kumulierte Zeit: {stats['cumulative_time']:.4f} Sekunden\n")

        # Optimierungsvorschläge
        report.append("Optimierungsvorschläge:")
        for suggestion in self.results['optimization_suggestions']:
            report.append(f"  - {suggestion}")

        return "\n".join(report)


# Beispiel für die Verwendung
def example_usage():
    # Beispiel-Code zum Testen
    test_code = """
def fibonacci(n):
    if n <= 1:
        return n
    else:
        return fibonacci(n-1) + fibonacci(n-2)

result = fibonacci(10)
"""

    optimizer = CodeOptimizer()
    analysis = optimizer.analyze_code(test_code)

    report = CodeOptimizationReport(analysis)
    print(report.generate_report())


if __name__ == "__main__":
    example_usage()
import ast
import time
import cProfile
import pstats
from typing import List, Dict, Any
from dataclasses import dataclass
from memory_profiler import profile
