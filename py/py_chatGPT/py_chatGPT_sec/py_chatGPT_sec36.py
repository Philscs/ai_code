from queue import PriorityQueue, Queue
from threading import Lock, Thread
import time
import uuid

class Event:
    def __init__(self, event_type, data, timestamp=None):
        self.event_id = str(uuid.uuid4())
        self.event_type = event_type
        self.data = data
        self.timestamp = timestamp or time.time()

    def __lt__(self, other):
        return self.timestamp < other.timestamp

class EventDispatcher:
    def __init__(self):
        self.event_handlers = {}
        self.event_log = []  # Event sourcing log
        self.dead_letter_queue = Queue()
        self.lock = Lock()
        self.event_queue = PriorityQueue()  # Ensure event ordering

    def register_handler(self, event_type, handler):
        if event_type not in self.event_handlers:
            self.event_handlers[event_type] = []
        self.event_handlers[event_type].append(handler)

    def emit(self, event):
        with self.lock:
            self.event_log.append(event)  # Store event for replay
            self.event_queue.put(event)  # Add to processing queue

    def replay_events(self):
        with self.lock:
            for event in sorted(self.event_log, key=lambda e: e.timestamp):
                self._dispatch(event)

    def _dispatch(self, event):
        handlers = self.event_handlers.get(event.event_type, [])
        if not handlers:
            self.dead_letter_queue.put(event)  # No handlers available
            return

        for handler in handlers:
            try:
                handler(event)
            except Exception as e:
                print(f"Error handling event {event.event_id}: {e}")
                self.dead_letter_queue.put(event)

    def start_dispatch_loop(self):
        def dispatch_loop():
            while True:
                event = self.event_queue.get()
                if event is None:  # Stop signal
                    break
                self._dispatch(event)

        self.dispatch_thread = Thread(target=dispatch_loop, daemon=True)
        self.dispatch_thread.start()

    def stop_dispatch_loop(self):
        self.event_queue.put(None)  # Send stop signal
        self.dispatch_thread.join()

    def process_dead_letters(self):
        while not self.dead_letter_queue.empty():
            event = self.dead_letter_queue.get()
            print(f"Dead letter event: {event.event_id}, type: {event.event_type}")

# Beispiel
if __name__ == "__main__":
    dispatcher = EventDispatcher()

    def sample_handler(event):
        print(f"Handled event: {event.event_id}, data: {event.data}")

    dispatcher.register_handler("example", sample_handler)
    dispatcher.start_dispatch_loop()

    event1 = Event("example", {"key": "value1"})
    event2 = Event("example", {"key": "value2"})
    dispatcher.emit(event1)
    dispatcher.emit(event2)

    time.sleep(1)  # Give some time for processing

    dispatcher.stop_dispatch_loop()
    dispatcher.replay_events()
    dispatcher.process_dead_letters()
