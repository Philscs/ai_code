import sys
import tracemalloc
import gc
import weakref
import psutil
import logging
from typing import Dict, Set, Optional
from datetime import datetime
from contextlib import contextmanager

class MemoryProfiler:
    def __init__(self, threshold_mb: float = 100, log_file: str = "memory_profile.log"):
        self.threshold_bytes = threshold_mb * 1024 * 1024
        self.tracked_objects: Dict[int, weakref.ref] = {}
        self.leaked_objects: Set[int] = set()
        self.logger = self._configure_logger(log_file)
        tracemalloc.start()

    def _configure_logger(self, log_file: str) -> logging.Logger:
        logging.basicConfig(
            filename=log_file,
            level=logging.INFO,
            format='%(asctime)s - %(levelname)s - %(message)s'
        )
        return logging.getLogger(__name__)

    def start_tracking(self, obj: object, name: str = "") -> None:
        obj_id = id(obj)
        self.tracked_objects[obj_id] = weakref.ref(obj)
        self.logger.info(f"Started tracking object {name} (id: {obj_id})")
        self._check_memory_usage(obj, name)

    def stop_tracking(self, obj: object) -> None:
        obj_id = id(obj)
        if obj_id in self.tracked_objects:
            del self.tracked_objects[obj_id]
            self._cleanup_object(obj)
            self.logger.info(f"Stopped tracking object (id: {obj_id})")

    def _check_memory_usage(self, obj: object, name: str) -> None:
        size = sys.getsizeof(obj)
        if size > self.threshold_bytes:
            self.logger.warning(
                f"Memory warning: Object {name} (id: {id(obj)}) "
                f"consumes {size / 1024 / 1024:.2f} MB"
            )

    def _cleanup_object(self, obj: object) -> None:
        try:
            if hasattr(obj, '__del__'):
                obj.__del__()
            gc.collect()
        except Exception as e:
            self.logger.error(f"Error during cleanup: {str(e)}")

    def check_for_leaks(self) -> None:
        current_objects = set(id(obj) for obj in gc.get_objects())
        for obj_id, weak_ref in list(self.tracked_objects.items()):
            obj = weak_ref()
            if obj is None or obj_id not in current_objects:
                self.leaked_objects.add(obj_id)
                self.logger.warning(f"Potential memory leak detected: Object {obj_id}")

    def get_memory_stats(self) -> dict:
        process = psutil.Process()
        stats = {
            'total_tracked_objects': len(self.tracked_objects),
            'potential_leaks': len(self.leaked_objects),
            'process_memory_mb': process.memory_info().rss / 1024 / 1024,
            'timestamp': datetime.now().isoformat()
        }
        return stats

    @contextmanager
    def profile_block(self, name: str = ""):
        snapshot1 = tracemalloc.take_snapshot()
        try:
            yield
        finally:
            snapshot2 = tracemalloc.take_snapshot()
            stats = snapshot2.compare_to(snapshot1, 'lineno')
            for stat in stats[:3]:
                self.logger.info(f"{name} - Memory block stat: {stat}")

# Usage example
if __name__ == "__main__":
    profiler = MemoryProfiler(threshold_mb=50)
    
    # Example for object tracking
    large_list = list(range(1000000))
    profiler.start_tracking(large_list, "large_list")
    
    # Example for profiling block
    with profiler.profile_block("data_processing"):
        data = [i ** 2 for i in range(100000)]
    
    # Check for memory leaks
    profiler.check_for_leaks()
    
    # Print memory stats
    stats = profiler.get_memory_stats()
    print(f"Memory stats: {stats}")
    
    # Cleanup
    profiler.stop_tracking(large_list)
