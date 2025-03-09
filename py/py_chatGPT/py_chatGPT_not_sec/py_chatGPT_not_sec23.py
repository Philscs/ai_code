import timeit
import ast
import astor
import line_profiler

class CodeAnalyzer(ast.NodeVisitor):
    """Analyzes the provided Python code for potential performance bottlenecks."""

    def __init__(self):
        self.loops = []

    def visit_For(self, node):
        self.loops.append(node)
        self.generic_visit(node)

    def visit_While(self, node):
        self.loops.append(node)
        self.generic_visit(node)

class CodeOptimizer:
    """Optimizes code based on the analysis."""

    def __init__(self, code):
        self.code = code
        self.tree = ast.parse(code)

    def analyze_code(self):
        analyzer = CodeAnalyzer()
        analyzer.visit(self.tree)
        return analyzer.loops

    def profile_code(self):
        """Profiles the provided code and identifies slow functions."""
        profiler = line_profiler.LineProfiler()
        exec_globals = {}
        exec(self.code, exec_globals)
        
        for name, func in exec_globals.items():
            if callable(func):
                profiler.add_function(func)

        profiler.runctx(self.code, exec_globals, exec_globals)
        profiler.print_stats()

    def suggest_optimizations(self, loops):
        """Suggest optimizations based on loop structures."""
        suggestions = []
        for loop in loops:
            if isinstance(loop, ast.For):
                suggestions.append("Consider using generator expressions or optimizing the iterable in the for loop.")
            elif isinstance(loop, ast.While):
                suggestions.append("Check if the while loop has a clear termination condition and optimize calculations within the loop.")
        return suggestions

    def optimize(self):
        loops = self.analyze_code()
        print("\nPerformance Analysis:")
        self.profile_code()
        
        print("\nOptimization Suggestions:")
        for suggestion in self.suggest_optimizations(loops):
            print(f"- {suggestion}")

if __name__ == "__main__":
    # Beispielcode zur Analyse
    sample_code = """
def example_function():
    total = 0
    for i in range(1000000):
        total += i
    return total

example_function()
"""
    
    optimizer = CodeOptimizer(sample_code)
    optimizer.optimize()
