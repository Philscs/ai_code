// Event class to represent domain events
class Event {
    constructor(type, data, aggregateId, version) {
      this.type = type;
      this.data = data;
      this.aggregateId = aggregateId;
      this.version = version;
      this.timestamp = new Date();
    }
  }
  
  // EventStore for storing and retrieving events
  class EventStore {
    constructor() {
      this.events = new Map(); // aggregateId -> events[]
    }
  
    append(event) {
      if (!this.events.has(event.aggregateId)) {
        this.events.set(event.aggregateId, []);
      }
      this.events.get(event.aggregateId).push(event);
    }
  
    getEvents(aggregateId, fromVersion = 0) {
      return (this.events.get(aggregateId) || [])
        .filter(event => event.version > fromVersion);
    }
  
    getAllEvents() {
      return Array.from(this.events.values()).flat();
    }
  }
  
  // SnapshotStore for managing aggregate snapshots
  class SnapshotStore {
    constructor() {
      this.snapshots = new Map(); // aggregateId -> {state, version}
    }
  
    save(aggregateId, state, version) {
      this.snapshots.set(aggregateId, { state, version });
    }
  
    get(aggregateId) {
      return this.snapshots.get(aggregateId);
    }
  }
  
  // Base class for aggregates
  class Aggregate {
    constructor(id) {
      this.id = id;
      this.version = 0;
      this.changes = [];
    }
  
    applyEvent(event) {
      this.version = event.version;
      this.handle(event);
    }
  
    applyChange(event) {
      this.changes.push(event);
      this.handle(event);
    }
  
    handle(event) {
      const handler = `on${event.type}`;
      if (typeof this[handler] === 'function') {
        this[handler](event.data);
      }
    }
  
    getUncommittedChanges() {
      return this.changes;
    }
  
    clearUncommittedChanges() {
      this.changes = [];
    }
  }
  
  // EventProcessor for handling event replay and projections
  class EventProcessor {
    constructor(eventStore, snapshotStore) {
      this.eventStore = eventStore;
      this.snapshotStore = snapshotStore;
      this.projections = new Map(); // projectionName -> projection function
    }
  
    // Load aggregate with optional snapshot
    loadAggregate(aggregateId, AggregateClass) {
      const snapshot = this.snapshotStore.get(aggregateId);
      const aggregate = new AggregateClass(aggregateId);
  
      if (snapshot) {
        Object.assign(aggregate, snapshot.state);
        aggregate.version = snapshot.version;
      }
  
      const events = this.eventStore.getEvents(aggregateId, aggregate.version);
      events.forEach(event => aggregate.applyEvent(event));
  
      return aggregate;
    }
  
    // Register a new projection
    registerProjection(name, projectionFn) {
      this.projections.set(name, projectionFn);
    }
  
    // Update all projections with new events
    updateProjections(events) {
      for (const [name, projectionFn] of this.projections) {
        events.forEach(event => projectionFn(event));
      }
    }
  
    // Replay all events from scratch
    replayEvents() {
      const events = this.eventStore.getAllEvents();
      events.sort((a, b) => a.timestamp - b.timestamp);
      
      // Clear existing projections
      this.projections.clear();
      
      // Replay events
      events.forEach(event => {
        this.updateProjections([event]);
      });
    }
  }
  
  // CommandHandler for processing commands
  class CommandHandler {
    constructor(eventStore, eventProcessor) {
      this.eventStore = eventStore;
      this.eventProcessor = eventProcessor;
      this.handlers = new Map(); // commandType -> handler function
    }
  
    register(commandType, handler) {
      this.handlers.set(commandType, handler);
    }
  
    async execute(command) {
      const handler = this.handlers.get(command.type);
      if (!handler) {
        throw new Error(`No handler registered for command type: ${command.type}`);
      }
  
      // Load aggregate
      const aggregate = await handler(command, this.eventProcessor);
  
      // Store new events
      const newEvents = aggregate.getUncommittedChanges();
      newEvents.forEach(event => {
        event.version = aggregate.version + 1;
        this.eventStore.append(event);
      });
  
      // Update projections
      this.eventProcessor.updateProjections(newEvents);
  
      // Clear uncommitted changes
      aggregate.clearUncommittedChanges();
  
      return aggregate;
    }
  }
  
  // Example usage: Bank Account
  
  // 1. Define Account Aggregate
  class Account extends Aggregate {
    constructor(id) {
      super(id);
      this.balance = 0;
    }
  
    onDeposit(data) {
      this.balance += data.amount;
    }
  
    onWithdraw(data) {
      this.balance -= data.amount;
    }
  
    deposit(amount) {
      this.applyChange(new Event('Deposit', { amount }, this.id, this.version));
    }
  
    withdraw(amount) {
      if (amount > this.balance) {
        throw new Error('Insufficient funds');
      }
      this.applyChange(new Event('Withdraw', { amount }, this.id, this.version));
    }
  }
  
  // 2. Set up the framework
  const eventStore = new EventStore();
  const snapshotStore = new SnapshotStore();
  const eventProcessor = new EventProcessor(eventStore, snapshotStore);
  const commandHandler = new CommandHandler(eventStore, eventProcessor);
  
  // 3. Register command handlers
  commandHandler.register('CreateAccount', async (command, processor) => {
    const account = new Account(command.aggregateId);
    return account;
  });
  
  commandHandler.register('Deposit', async (command, processor) => {
    const account = processor.loadAggregate(command.aggregateId, Account);
    account.deposit(command.data.amount);
    return account;
  });
  
  commandHandler.register('Withdraw', async (command, processor) => {
    const account = processor.loadAggregate(command.aggregateId, Account);
    account.withdraw(command.data.amount);
    return account;
  });
  
  // 4. Register projections
  eventProcessor.registerProjection('balances', (event) => {
    // Update account balance projection
    if (event.type === 'Deposit' || event.type === 'Withdraw') {
      // Update read model...
    }
  });
  
  // 5. Example usage
  async function example() {
    try {
      // Create account
      await commandHandler.execute({
        type: 'CreateAccount',
        aggregateId: 'acc-123'
      });
  
      // Deposit money
      await commandHandler.execute({
        type: 'Deposit',
        aggregateId: 'acc-123',
        data: { amount: 100 }
      });
  
      // Withdraw money
      await commandHandler.execute({
        type: 'Withdraw',
        aggregateId: 'acc-123',
        data: { amount: 30 }
      });
  
      // Load account state
      const account = eventProcessor.loadAggregate('acc-123', Account);
      console.log('Final balance:', account.balance); // 70
  
      // Create snapshot
      snapshotStore.save('acc-123', account, account.version);
    } catch (error) {
      console.error('Error:', error.message);
    }
  }
  
  // Export framework components
  export {
    Event,
    EventStore,
    SnapshotStore,
    Aggregate,
    EventProcessor,
    CommandHandler,
    Account // Example aggregate
  };