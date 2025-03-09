import threading
from queue import Queue

class Event:
    def __init__(self, name, data):
        self.name = name
        self.data = data

class EventDispatcher:
    def __init__(self):
        self.event_queue = Queue()
        self.event_handlers = {}
        self.dead_letter_queue = Queue()

    def add_event_handler(self, event_name, handler):
        if event_name not in self.event_handlers:
            self.event_handlers[event_name] = []
        self.event_handlers[event_name].append(handler)

    def dispatch_event(self, event):
        if event.name in self.event_handlers:
            for handler in self.event_handlers[event.name]:
                handler(event.data)
        else:
            self.dead_letter_queue.put(event)

    def replay_events(self):
        while not self.event_queue.empty():
            event = self.event_queue.get()
            self.dispatch_event(event)

    def start_event_loop(self):
        while True:
            event = self.event_queue.get()
            self.dispatch_event(event)

    def enqueue_event(self, event):
        self.event_queue.put(event)

    def handle_dead_letters(self):
        while not self.dead_letter_queue.empty():
            event = self.dead_letter_queue.get()
            print(f"Dead letter: {event.name} - {event.data}")

# Example usage
def handle_event(data):
    print(f"Handling event: {data}")

dispatcher = EventDispatcher()
dispatcher.add_event_handler("event1", handle_event)
dispatcher.add_event_handler("event2", handle_event)

event1 = Event("event1", "data1")
event2 = Event("event2", "data2")
event3 = Event("event3", "data3")

dispatcher.enqueue_event(event1)
dispatcher.enqueue_event(event2)
dispatcher.enqueue_event(event3)

dispatcher.replay_events()
dispatcher.handle_dead_letters()

# Start the event loop in a separate thread
event_loop_thread = threading.Thread(target=dispatcher.start_event_loop)
event_loop_thread.start()
