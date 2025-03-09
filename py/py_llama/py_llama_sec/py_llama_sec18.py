import gc
import os
import psutil
from datetime import datetime
from typing import Dict, List

class MemoryProfiler:
    def __init__(self):
        self.peak_memory = 0
        self.current_memory = 0
        self.memory_leaks = {}
        self.warning_count = 0

    def get_memory_usage(self) -> int:
        return psutil.Process(os.getpid()).memory_info().rss / (1024 * 1024)

    def collect_data(self):
        self.peak_memory = max(self.peak_memory, self.get_memory_usage())
        self.current_memory += self.get_memory_usage()
        self.memory_leaks.update({datetime.now(): self.get_memory_usage()})

    def detect_leaks(self) -> Dict:
        # Cleanup nach 5 Sekunden
        if datetime.now() - self.memory_leaks[datetime.now()] > timedelta(seconds=5):
            del self.memory_leaks[datetime.now()]
            print(f"Speicherleck gefunden! {self.get_memory_usage()} MB verloren")

    def check_leak_threshold(self) -> bool:
        if len(self.memory_leaks) >= 10:
            return True
        return False

    def generate_warnings(self):
        if self.current_memory > 1000 * 1024:  # 1 GB
            print(f"Warnung! Speicherverbrauch übersteigt die Warnschwelle (1 GB).")
            self.warning_count += 1
        elif self.warning_count >= 5:
            print("Warnung! Zu viele Warnungen generiert.")

    def cleanup(self):
        # Vermeiden Sie, dass der Profiler nicht gelöscht wird
        import gc; gc.collect()
        # Löse alle Objekte auf, die mehr als 5 Sekunden lang überdauert haben
        for key in list(self.memory_leaks.keys()):
            if datetime.now() - key > timedelta(seconds=5):
                del self.memory_leaks[key]
        # Lösen Sie alle Mem-Objekte
        import gc; gc.collect()

def main():
    profiler = MemoryProfiler()
    while True:
        profiler.collect_data()
        profiler.detect_leaks()
        if profiler.check_leak_threshold():
            print("Speicherleck erkannt!")
        profiler.generate_warnings()
        # Warten 1 Sekunde
        time.sleep(1)

if __name__ == "__main__":
    main()
