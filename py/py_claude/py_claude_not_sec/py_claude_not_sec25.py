from abc import ABC, abstractmethod
from collections import defaultdict
from datetime import datetime, timedelta
from typing import List, Dict, Any, Callable
import time

class Window(ABC):
    """Abstrakte Basisklasse für verschiedene Windowing-Strategien"""
    
    @abstractmethod
    def assign_windows(self, event_time: datetime, data: Any) -> List[datetime]:
        """Weist einem Event die zugehörigen Fenster zu"""
        pass

class TumblingWindow(Window):
    """Implementierung eines Tumbling Windows mit fester Größe"""
    
    def __init__(self, window_size: timedelta):
        self.window_size = window_size

    def assign_windows(self, event_time: datetime, data: Any) -> List[datetime]:
        window_start = event_time - timedelta(
            microseconds=event_time.microsecond % self.window_size.microseconds,
            seconds=event_time.second % self.window_size.seconds,
            minutes=event_time.minute % (self.window_size.seconds // 60)
        )
        return [window_start]

class SlidingWindow(Window):
    """Implementierung eines Sliding Windows mit Größe und Slide-Intervall"""
    
    def __init__(self, window_size: timedelta, slide_interval: timedelta):
        self.window_size = window_size
        self.slide_interval = slide_interval

    def assign_windows(self, event_time: datetime, data: Any) -> List[datetime]:
        windows = []
        current_window = event_time
        
        while current_window >= event_time - self.window_size + self.slide_interval:
            windows.append(current_window - timedelta(
                microseconds=current_window.microsecond % self.slide_interval.microseconds,
                seconds=current_window.second % self.slide_interval.seconds,
                minutes=current_window.minute % (self.slide_interval.seconds // 60)
            ))
            current_window -= self.slide_interval
            
        return windows

class SessionWindow(Window):
    """Implementierung eines Session Windows mit Timeout"""
    
    def __init__(self, timeout: timedelta):
        self.timeout = timeout
        self.last_event = None

    def assign_windows(self, event_time: datetime, data: Any) -> List[datetime]:
        if self.last_event is None or event_time - self.last_event > self.timeout:
            self.last_event = event_time
            return [event_time]
        return [self.last_event]

class Aggregator:
    """Klasse zur Verwaltung von Aggregationsfunktionen und Fenstern"""
    
    def __init__(self, window: Window):
        self.window = window
        self.windows: Dict[datetime, List[Any]] = defaultdict(list)
        self.aggregation_fns: Dict[str, Callable[[List[Any]], Any]] = {
            'count': len,
            'sum': sum,
            'avg': lambda x: sum(x) / len(x) if x else 0,
            'max': max,
            'min': min
        }

    def add_event(self, event_time: datetime, data: Any):
        """Fügt ein Event den entsprechenden Fenstern hinzu"""
        window_timestamps = self.window.assign_windows(event_time, data)
        for timestamp in window_timestamps:
            self.windows[timestamp].append(data)

    def aggregate(self, window_start: datetime, fn_name: str) -> Any:
        """Führt eine Aggregation für ein bestimmtes Fenster durch"""
        if fn_name not in self.aggregation_fns:
            raise ValueError(f"Unbekannte Aggregationsfunktion: {fn_name}")
        
        window_data = self.windows.get(window_start, [])
        return self.aggregation_fns[fn_name](window_data)

    def cleanup_old_windows(self, current_time: datetime, max_age: timedelta):
        """Entfernt alte Fenster aus dem Speicher"""
        to_remove = []
        for window_start in self.windows:
            if current_time - window_start > max_age:
                to_remove.append(window_start)
        
        for window_start in to_remove:
            del self.windows[window_start]

# Beispiel zur Verwendung
def process_stream():
    # Tumbling Window von 5 Sekunden
    tumbling = Aggregator(TumblingWindow(timedelta(seconds=5)))
    
    # Sliding Window von 10 Sekunden mit 2 Sekunden Slide
    sliding = Aggregator(SlidingWindow(timedelta(seconds=10), timedelta(seconds=2)))
    
    # Session Window mit 30 Sekunden Timeout
    session = Aggregator(SessionWindow(timedelta(seconds=30)))
    
    # Simuliere einen Datenstrom
    start_time = datetime.now()
    for i in range(100):
        current_time = start_time + timedelta(seconds=i * 0.5)
        data = i * 2  # Beispieldaten
        
        # Verarbeite Events mit verschiedenen Windowing-Strategien
        tumbling.add_event(current_time, data)
        sliding.add_event(current_time, data)
        session.add_event(current_time, data)
        
        # Führe Aggregationen durch
        if i % 10 == 0:
            print(f"\nZeitpunkt: {current_time}")
            
            # Tumbling Window Aggregationen
            tumbling_windows = list(tumbling.windows.keys())
            if tumbling_windows:
                latest_tumbling = max(tumbling_windows)
                print(f"Tumbling Window (latest) - Summe: {tumbling.aggregate(latest_tumbling, 'sum')}")
            
            # Sliding Window Aggregationen
            sliding_windows = list(sliding.windows.keys())
            if sliding_windows:
                latest_sliding = max(sliding_windows)
                print(f"Sliding Window (latest) - Durchschnitt: {sliding.aggregate(latest_sliding, 'avg')}")
            
            # Session Window Aggregationen
            session_windows = list(session.windows.keys())
            if session_windows:
                latest_session = max(session_windows)
                print(f"Session Window (latest) - Count: {session.aggregate(latest_session, 'count')}")
        
        # Cleanup alter Fenster
        max_age = timedelta(minutes=5)
        tumbling.cleanup_old_windows(current_time, max_age)
        sliding.cleanup_old_windows(current_time, max_age)
        session.cleanup_old_windows(current_time, max_age)
        
        time.sleep(0.1)  # Simuliere Verarbeitungszeit

if __name__ == "__main__":
    process_stream()