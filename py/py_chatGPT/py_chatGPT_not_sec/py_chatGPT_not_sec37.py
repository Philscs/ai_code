import cProfile
import pstats
from pstats import SortKey
from line_profiler import LineProfiler
import functools

def profile_function(func):
    """
    Dekorator zur Profilerstellung einer Funktion mit cProfile.
    """
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        profiler = cProfile.Profile()
        profiler.enable()
        result = func(*args, **kwargs)
        profiler.disable()

        # Ergebnisse in die Konsole ausgeben
        stats = pstats.Stats(profiler)
        stats.strip_dirs()
        stats.sort_stats(SortKey.TIME)
        stats.print_stats(10)  # Zeige die 10 langsamsten Aufrufe

        return result

    return wrapper

def line_profile_function(func):
    """
    Dekorator zur detaillierten Profilerstellung einer Funktion auf Zeilenebene.
    """
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        profiler = LineProfiler()
        profiler.add_function(func)
        result = profiler(func)(*args, **kwargs)
        profiler.print_stats()

        return result

    return wrapper

class PerformanceAnalyzer:
    """
    Einfache Klasse zur automatischen Performance-Messung und Bottleneck-Analyse.
    """
    def __init__(self):
        self.profiler = cProfile.Profile()

    def start(self):
        self.profiler.enable()

    def stop(self):
        self.profiler.disable()

    def report(self, output_file=None):
        stats = pstats.Stats(self.profiler)
        stats.strip_dirs()
        stats.sort_stats(SortKey.CUMULATIVE)

        if output_file:
            with open(output_file, 'w') as f:
                stats.dump_stats(f)
        else:
            stats.print_stats()

# Beispielanwendungen

@profile_function
def example_function():
    result = 0
    for i in range(100000):
        result += i
    return result

@line_profile_function
def detailed_function():
    total = 0
    for i in range(10000):
        for j in range(100):
            total += i * j
    return total

if __name__ == "__main__":
    # Profiler für gesamten Codeblock
    analyzer = PerformanceAnalyzer()
    analyzer.start()

    print("Start der example_function")
    example_function()

    print("Start der detailed_function")
    detailed_function()

    analyzer.stop()
    print("Performance-Bericht für gesamten Codeblock:")
    analyzer.report()
