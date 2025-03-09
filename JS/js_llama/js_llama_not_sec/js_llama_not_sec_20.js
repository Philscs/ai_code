// event-sourcing-framework.js
class EventStore {
    constructor() {
      this.events = [];
      this.snapshots = {};
    }
  
    saveEvent(event) {
      this.events.push(event);
      this.saveSnapshot();
    }
  
    saveSnapshot() {
      const snapshot = { timestamp: Date.now(), events: [...this.events] };
      this.snapshots[Date.now()] = snapshot;
    }
  
    getEventsByTimestamp(timestamp) {
      return this.events.filter((event) => event.timestamp <= timestamp);
    }
  
    getSnapshotByTimestamp(timestamp) {
      if (timestamp in this.snapshots) {
        return this.snapshots[timestamp];
      }
      return null;
    }
  }
  
  class Event {
    constructor(id, data, timestamp = Date.now()) {
      this.id = id;
      this.data = data;
      this.timestamp = timestamp;
    }
  }
  
  class CommandHandler {
    constructor(eventStore) {
      this.eventStore = eventStore;
    }
  
    handleCommand(command) {
      const event = new Event(command.id, { ...command, type: 'command' });
      return this.eventStore.saveEvent(event);
    }
  }
  
  class Projection {
    constructor(eventStore) {
      this.eventStore = eventStore;
    }
  
    applyEvents(events) {
      const result = [];
      events.forEach((event) => {
        if (event.type === 'command') {
          // Handle command events here, e.g., execute the command
        } else if (event.type === 'domainEvent') {
          // Handle domain event events here, e.g., apply business logic
        }
        result.push(event.data);
      });
      return result;
    }
  
    getProjectionSnapshot() {
      const snapshot = this.eventStore.getSnapshotByTimestamp(Date.now());
      if (snapshot) {
        return snapshot.events.reduce((acc, event) => {
          acc[event.id] = event.data;
          return acc;
        }, {});
      }
      return {};
    }
  }
  
  class CommandHandlerWithProjection extends CommandHandler {
    constructor(eventStore, projection) {
      super(eventStore);
      this.projection = projection;
    }
  
    handleCommand(command) {
      const event = new Event(command.id, { ...command, type: 'command' });
      return this.eventStore.saveEvent(event).then(() => {
        return this.projection.applyEvents(this.eventStore.getEventsByTimestamp(Date.now()));
      });
    }
  }
  
  const createEventStore = () => {
    const eventStore = new EventStore();
    return {
      saveEvent: (event) => eventStore.saveEvent(event),
      getEventsByTimestamp: (timestamp) => eventStore.getEventsByTimestamp(timestamp),
      getSnapshotByTimestamp: (timestamp) => eventStore.getSnapshotByTimestamp(timestamp)
    };
  };
  
  const createCommandHandlerWithProjection = (eventStore, projection) => {
    return new CommandHandlerWithProjection(eventStore, projection);
  };
  
  // Usage
  const eventStore = createEventStore();
  const projection = new Projection(eventStore);
  const commandHandler = createCommandHandlerWithProjection(eventStore, projection);
  
  const command = { id: '123', type: 'command', data: { name: 'John Doe' } };
  eventStore.saveEvent(command).then(() => {
    const projectionSnapshot = commandHandler.projection.getProjectionSnapshot();
    console.log(projectionSnapshot); // Output: { 123: { name: 'John Doe' } }
  });
  