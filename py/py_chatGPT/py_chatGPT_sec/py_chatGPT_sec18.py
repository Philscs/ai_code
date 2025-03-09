import gc
import os
import psutil
import weakref
from collections import defaultdict

class MemoryProfiler:
    def __init__(self):
        self._tracked_objects = weakref.WeakKeyDictionary()
        self._initial_memory = self._get_process_memory()

    def _get_process_memory(self):
        """Returns the memory usage of the current process in bytes."""
        process = psutil.Process(os.getpid())
        return process.memory_info().rss

    def track_object(self, obj, label=None):
        """Tracks an object and associates it with an optional label."""
        label = label or f"Object_{id(obj)}"
        self._tracked_objects[obj] = label

    def analyze_memory(self):
        """Analyzes the memory usage of tracked objects and detects potential issues."""
        memory_by_label = defaultdict(int)
        
        for obj, label in self._tracked_objects.items():
            try:
                memory_by_label[label] += sys.getsizeof(obj)
            except TypeError:
                pass  # Handle cases where object size cannot be determined

        leaks = []
        for label, memory in memory_by_label.items():
            if memory > 1024 * 1024:  # Example threshold: 1MB
                leaks.append((label, memory))

        return memory_by_label, leaks

    def cleanup(self):
        """Cleans up references to tracked objects and forces garbage collection."""
        self._tracked_objects.clear()
        gc.collect()

    def generate_report(self):
        """Generates a report of memory usage and potential leaks."""
        current_memory = self._get_process_memory()
        memory_by_label, leaks = self.analyze_memory()

        report = {
            "Initial Memory (bytes)": self._initial_memory,
            "Current Memory (bytes)": current_memory,
            "Tracked Objects": len(self._tracked_objects),
            "Memory Usage by Label": memory_by_label,
            "Potential Leaks": leaks,
        }

        return report

# Example usage
if __name__ == "__main__":
    profiler = MemoryProfiler()

    # Track some objects
    a = [1] * (10**6)  # Large list
    b = {"key": "value" * 1000}  # Large dictionary

    profiler.track_object(a, "Large List")
    profiler.track_object(b, "Large Dictionary")

    # Analyze memory
    report = profiler.generate_report()

    print("Memory Profiling Report:")
    for key, value in report.items():
        print(f"{key}: {value}")

    # Cleanup
    profiler.cleanup()
