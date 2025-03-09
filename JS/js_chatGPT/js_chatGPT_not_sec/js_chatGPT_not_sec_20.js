class EventStore {
    constructor() {
      this.events = [];
    }
  
    appendEvent(event) {
      this.events.push(event);
    }
  
    getEvents(aggregateId) {
      return this.events.filter(event => event.aggregateId === aggregateId);
    }
  }
  
  class SnapshotStore {
    constructor() {
      this.snapshots = new Map();
    }
  
    saveSnapshot(aggregateId, snapshot) {
      this.snapshots.set(aggregateId, snapshot);
    }
  
    getSnapshot(aggregateId) {
      return this.snapshots.get(aggregateId) || null;
    }
  }
  
  class EventSourcing {
    constructor(eventStore, snapshotStore) {
      this.eventStore = eventStore;
      this.snapshotStore = snapshotStore;
    }
  
    saveEvent(event) {
      this.eventStore.appendEvent(event);
    }
  
    replayEvents(aggregateId, applyEventFn) {
      const events = this.eventStore.getEvents(aggregateId);
      events.forEach(applyEventFn);
    }
  
    applySnapshot(aggregateId, initializeFromSnapshot) {
      const snapshot = this.snapshotStore.getSnapshot(aggregateId);
      if (snapshot) {
        initializeFromSnapshot(snapshot);
      }
    }
  }
  
  class Projection {
    constructor() {
      this.data = {};
    }
  
    project(event) {
      // Define how projections should be updated based on events
      if (event.type === "ITEM_ADDED") {
        this.data[event.payload.itemId] = event.payload;
      } else if (event.type === "ITEM_UPDATED") {
        Object.assign(this.data[event.payload.itemId], event.payload);
      }
    }
  
    getProjection() {
      return this.data;
    }
  }
  
  class CommandHandler {
    constructor(eventSourcing) {
      this.eventSourcing = eventSourcing;
    }
  
    handle(command) {
      const { aggregateId, type, payload } = command;
  
      if (type === "ADD_ITEM") {
        const event = {
          aggregateId,
          type: "ITEM_ADDED",
          payload,
          timestamp: Date.now(),
        };
        this.eventSourcing.saveEvent(event);
      } else if (type === "UPDATE_ITEM") {
        const event = {
          aggregateId,
          type: "ITEM_UPDATED",
          payload,
          timestamp: Date.now(),
        };
        this.eventSourcing.saveEvent(event);
      }
    }
  }
  
  // Example usage
  const eventStore = new EventStore();
  const snapshotStore = new SnapshotStore();
  const eventSourcing = new EventSourcing(eventStore, snapshotStore);
  const projection = new Projection();
  const commandHandler = new CommandHandler(eventSourcing);
  
  // Handle commands
  commandHandler.handle({
    aggregateId: "123",
    type: "ADD_ITEM",
    payload: { itemId: "item1", name: "Test Item", quantity: 10 },
  });
  
  commandHandler.handle({
    aggregateId: "123",
    type: "UPDATE_ITEM",
    payload: { itemId: "item1", quantity: 20 },
  });
  
  // Replay events to build projection
  const events = eventStore.getEvents("123");
  events.forEach(event => projection.project(event));
  
  console.log(projection.getProjection());
  