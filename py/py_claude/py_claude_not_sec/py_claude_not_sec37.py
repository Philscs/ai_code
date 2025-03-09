import cProfile
import pstats
import time
import memory_profiler
import line_profiler
import functools
from typing import Callable, Any, Dict
import logging
from dataclasses import dataclass
from contextlib import contextmanager

# Konfiguration des Logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

@dataclass
class PerformanceMetrics:
    """Datenklasse zur Speicherung von Performance-Metriken"""
    execution_time: float
    memory_usage: float
    function_calls: int
    cpu_time: float

class PerformanceProfiler:
    def __init__(self, enabled: bool = True):
        self.enabled = enabled
        self.profiler = cProfile.Profile()
        self.metrics_history: Dict[str, list[PerformanceMetrics]] = {}

    def profile(self, func: Callable) -> Callable:
        """Decorator für das Profiling von Funktionen"""
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            if not self.enabled:
                return func(*args, **kwargs)

            # Zeit- und Speicherverbrauch messen
            start_time = time.perf_counter()
            start_memory = memory_profiler.memory_usage()[0]

            # CPU-Profiling starten
            self.profiler.enable()
            result = func(*args, **kwargs)
            self.profiler.disable()

            # Metriken berechnen
            end_time = time.perf_counter()
            end_memory = memory_profiler.memory_usage()[0]
            
            stats = pstats.Stats(self.profiler)
            metrics = PerformanceMetrics(
                execution_time=end_time - start_time,
                memory_usage=end_memory - start_memory,
                function_calls=stats.total_calls,
                cpu_time=stats.total_tt
            )

            # Metriken speichern
            if func.__name__ not in self.metrics_history:
                self.metrics_history[func.__name__] = []
            self.metrics_history[func.__name__].append(metrics)

            # Performance-Log erstellen
            logger.info(f"Performance Metrics für {func.__name__}:")
            logger.info(f"Ausführungszeit: {metrics.execution_time:.4f} Sekunden")
            logger.info(f"Speicherverbrauch: {metrics.memory_usage:.2f} MB")
            logger.info(f"Funktionsaufrufe: {metrics.function_calls}")
            logger.info(f"CPU-Zeit: {metrics.cpu_time:.4f} Sekunden")

            return result
        return wrapper

    def analyze_bottlenecks(self, function_name: str = None):
        """Analysiert Bottlenecks basierend auf gesammelten Metriken"""
        if function_name and function_name in self.metrics_history:
            metrics_list = self.metrics_history[function_name]
        else:
            metrics_list = [m for metrics in self.metrics_history.values() for m in metrics]

        if not metrics_list:
            logger.warning("Keine Metriken für die Analyse verfügbar")
            return

        # Durchschnittliche Metriken berechnen
        avg_time = sum(m.execution_time for m in metrics_list) / len(metrics_list)
        avg_memory = sum(m.memory_usage for m in metrics_list) / len(metrics_list)
        
        # Bottleneck-Analyse
        logger.info("\nBottleneck-Analyse:")
        if avg_time > 1.0:  # Schwellwert für langsame Ausführung
            logger.warning(f"Performance-Bottleneck: Hohe Ausführungszeit ({avg_time:.2f}s)")
        
        if avg_memory > 100:  # Schwellwert für hohen Speicherverbrauch (MB)
            logger.warning(f"Performance-Bottleneck: Hoher Speicherverbrauch ({avg_memory:.2f}MB)")

@contextmanager
def timing_block(name: str):
    """Context Manager für die Zeitmessung von Code-Blöcken"""
    start_time = time.perf_counter()
    try:
        yield
    finally:
        end_time = time.perf_counter()
        logger.info(f"Block '{name}' ausgeführt in {end_time - start_time:.4f} Sekunden")

# Beispielverwendung
if __name__ == "__main__":
    profiler = PerformanceProfiler()

    @profiler.profile
    def example_function(n: int):
        """Beispielfunktion für das Profiling"""
        result = 0
        for i in range(n):
            with timing_block(f"Iteration {i}"):
                result += sum(range(1000))  # Rechenintensive Operation
        return result

    # Funktion mehrmals ausführen
    for i in range(3):
        example_function(1000)

    # Bottleneck-Analyse durchführen
    profiler.analyze_bottlenecks("example_function")