import time
from typing import Dict, List
from enum import Enum

class EventType(Enum):
    CREATED = 1
    UPDATED = 2
    DELETED = 3

class Event:
    def __init__(self, event_type: EventType, payload: Dict):
        self.event_type = event_type
        self.payload = payload

class EventDispatcher:
    def __init__(self):
        self.events = []
        self.queue = []

    def dispatch(self, event: Event):
        self.events.append(event)
        if not self.queue:
            self.process_events()
        else:
            self.queue.append(event)

    def process_events(self):
        while len(self.events) > 0 and len(self.queue) == 0:
            event = self.events.pop(0)
            print(f"Processing event: {event.event_type.name} with payload: {event.payload}")
            # Simulate some time-consuming operation
            time.sleep(1)

    def replay_events(self):
        for event in self.queue:
            self.process_events()

    def handle DeadLetter(self, max_retries: int = 5, delay: float = 0.1):
        if len(self.queue) > 0:
            event = self.queue.pop(0)
            print(f"Received DeadLetter event: {event.event_type.name} with payload: {event.payload}")
            retries = 0
            while retries < max_retries and len(self.queue) == 0:
                time.sleep(delay)
                self.process_events()
                retries += 1

    def order_events(self, ordering_key: str):
        if len(self.events) > 0:
            event = self.events.pop(0)
            print(f"Processing ordered event: {event.event_type.name} with payload: 
{event.payload}")
            # Simulate some time-consuming operation
            time.sleep(1)

def main():
    dispatcher = EventDispatcher()

    # Dispatch events in different orders
    dispatcher.dispatch(Event(EventType.CREATED, {"id": 1}))
    dispatcher.order_events("sorted")
    dispatcher.dispatch(Event(EventType.UPDATED, {"id": 2}))
    dispatcher.order_events("sorted")
    dispatcher.dispatch(Event(EventType.DELETED, {"id": 3}))

if __name__ == "__main__":
    main()
