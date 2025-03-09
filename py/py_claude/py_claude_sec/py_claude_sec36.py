from typing import Dict, List, Any, Callable
from datetime import datetime
import threading
import queue
import logging
from dataclasses import dataclass
from abc import ABC, abstractmethod
import uuid
import pickle
from collections import defaultdict

# Event Basisklasse
@dataclass
class Event:
    event_id: str
    timestamp: datetime
    version: int
    payload: Any
    
    def __init__(self, payload: Any):
        self.event_id = str(uuid.uuid4())
        self.timestamp = datetime.now()
        self.version = 1
        self.payload = payload

# Event Store Interface
class EventStore(ABC):
    @abstractmethod
    def append(self, event: Event) -> None:
        pass
    
    @abstractmethod
    def get_events(self, start_timestamp: datetime = None) -> List[Event]:
        pass

# In-Memory Event Store Implementation
class InMemoryEventStore(EventStore):
    def __init__(self):
        self._events: List[Event] = []
        self._lock = threading.Lock()
    
    def append(self, event: Event) -> None:
        with self._lock:
            self._events.append(event)
    
    def get_events(self, start_timestamp: datetime = None) -> List[Event]:
        with self._lock:
            if start_timestamp is None:
                return self._events.copy()
            return [e for e in self._events if e.timestamp >= start_timestamp]

# Dead Letter Queue
class DeadLetterQueue:
    def __init__(self):
        self._queue = queue.Queue()
        self._failed_events: Dict[str, tuple[Event, Exception]] = {}
    
    def add(self, event: Event, error: Exception) -> None:
        self._failed_events[event.event_id] = (event, error)
        self._queue.put(event.event_id)
    
    def get_failed_event(self, event_id: str) -> tuple[Event, Exception]:
        return self._failed_events.get(event_id)
    
    def retry_event(self, event_id: str) -> Event:
        if event_id in self._failed_events:
            event, _ = self._failed_events.pop(event_id)
            return event
        return None

# Event Dispatcher
class EventDispatcher:
    def __init__(self, event_store: EventStore):
        self._event_store = event_store
        self._handlers: Dict[str, List[Callable]] = defaultdict(list)
        self._dead_letter_queue = DeadLetterQueue()
        self._processing_lock = threading.Lock()
        self._event_queues: Dict[str, queue.PriorityQueue] = defaultdict(queue.PriorityQueue)
        self._logger = logging.getLogger(__name__)
        
        # Worker Threads für Event-Verarbeitung
        self._workers: Dict[str, threading.Thread] = {}
        self._stop_event = threading.Event()
    
    def register_handler(self, event_type: str, handler: Callable) -> None:
        """Registriert einen Event Handler für einen bestimmten Event-Typ"""
        with self._processing_lock:
            self._handlers[event_type].append(handler)
            
            # Starte Worker Thread falls noch nicht vorhanden
            if event_type not in self._workers:
                worker = threading.Thread(
                    target=self._process_event_queue,
                    args=(event_type,),
                    daemon=True
                )
                self._workers[event_type] = worker
                worker.start()
    
    def publish(self, event: Event, event_type: str) -> None:
        """Publiziert ein Event"""
        try:
            # Persistiere Event
            self._event_store.append(event)
            
            # Füge Event zur Verarbeitungsqueue hinzu
            self._event_queues[event_type].put((event.timestamp.timestamp(), event))
            
        except Exception as e:
            self._logger.error(f"Fehler beim Publizieren des Events {event.event_id}: {str(e)}")
            self._dead_letter_queue.add(event, e)
    
    def _process_event_queue(self, event_type: str) -> None:
        """Verarbeitet Events eines bestimmten Typs in der richtigen Reihenfolge"""
        while not self._stop_event.is_set():
            try:
                # Hole nächstes Event aus der Queue
                _, event = self._event_queues[event_type].get(timeout=1.0)
                
                # Verarbeite Event mit allen registrierten Handlern
                for handler in self._handlers[event_type]:
                    try:
                        handler(event)
                    except Exception as e:
                        self._logger.error(
                            f"Fehler bei der Verarbeitung von Event {event.event_id} "
                            f"durch Handler {handler.__name__}: {str(e)}"
                        )
                        self._dead_letter_queue.add(event, e)
                
            except queue.Empty:
                continue
            except Exception as e:
                self._logger.error(f"Fehler in Event Processing Loop: {str(e)}")
    
    def replay_events(self, start_timestamp: datetime = None) -> None:
        """Replay aller Events ab einem bestimmten Zeitpunkt"""
        events = self._event_store.get_events(start_timestamp)
        
        # Sortiere Events nach Timestamp
        events.sort(key=lambda e: e.timestamp)
        
        for event in events:
            # Bestimme Event-Typ
            event_type = event.payload.__class__.__name__
            
            if event_type in self._handlers:
                # Verarbeite Event mit allen registrierten Handlern
                for handler in self._handlers[event_type]:
                    try:
                        handler(event)
                    except Exception as e:
                        self._logger.error(
                            f"Fehler beim Replay von Event {event.event_id}: {str(e)}"
                        )
    
    def retry_failed_event(self, event_id: str) -> bool:
        """Wiederholt die Verarbeitung eines fehlgeschlagenen Events"""
        event = self._dead_letter_queue.retry_event(event_id)
        if event:
            event_type = event.payload.__class__.__name__
            self.publish(event, event_type)
            return True
        return False
    
    def shutdown(self) -> None:
        """Beendet alle Worker Threads sauber"""
        self._stop_event.set()
        for worker in self._workers.values():
            worker.join(timeout=5.0)

# Beispiel Verwendung
if __name__ == "__main__":
    # Logging Setup
    logging.basicConfig(level=logging.INFO)
    
    # Event Store erstellen
    event_store = InMemoryEventStore()
    
    # Event Dispatcher erstellen
    dispatcher = EventDispatcher(event_store)
    
    # Beispiel Event Payload
    @dataclass
    class UserCreated:
        user_id: str
        username: str
    
    # Beispiel Event Handler
    def handle_user_created(event: Event) -> None:
        user = event.payload
        print(f"Neuer Benutzer erstellt: {user.username} (ID: {user.user_id})")
    
    # Handler registrieren
    dispatcher.register_handler("UserCreated", handle_user_created)
    
    # Event publizieren
    user_event = Event(UserCreated("123", "max.mustermann"))
    dispatcher.publish(user_event, "UserCreated")
    
    # Warte kurz auf Verarbeitung
    import time
    time.sleep(1)
    
    # Dispatcher herunterfahren
    dispatcher.shutdown()