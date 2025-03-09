class EventEmitter {
    constructor() {
      this.events = new Map();
    }
  
    on(event, callback, priority = 0) {
      if (!this.events.has(event)) {
        this.events.set(event, []);
      }
      this.events.get(event).push({ callback, priority });
      // Sort callbacks by priority (higher priority first)
      this.events.get(event).sort((a, b) => b.priority - a.priority);
    }
  
    emit(event, data) {
      if (this.events.has(event)) {
        const listeners = this.events.get(event);
        listeners.forEach(listener => {
          try {
            listener.callback(data);
          } catch (error) {
            console.error(`Error in '${event}' listener:`, error);
          }
        });
      }
    }
  
    once(event, callback, priority = 0) {
      const onceWrapper = (data) => {
        callback(data);
        this.off(event, onceWrapper);
      };
      this.on(event, onceWrapper, priority);
    }
  
    off(event, callback) {
      if (this.events.has(event)) {
        const filteredListeners = this.events
          .get(event)
          .filter(listener => listener.callback !== callback);
        this.events.set(event, filteredListeners);
        if (filteredListeners.length === 0) {
          this.events.delete(event);
        }
      }
    }
  
    async emitAsync(event, data) {
      if (this.events.has(event)) {
        const listeners = this.events.get(event);
        for (const listener of listeners) {
          try {
            await listener.callback(data);
          } catch (error) {
            console.error(`Error in '${event}' listener:`, error);
          }
        }
      }
    }
  
    // Memory leak prevention by limiting listeners
    setMaxListeners(event, maxListeners) {
      if (!this.events.has(event)) {
        this.events.set(event, []);
      }
      this.events.get(event).maxListeners = maxListeners;
    }
  
    onWithLimit(event, callback, limit, priority = 0) {
      if (!this.events.has(event)) {
        this.events.set(event, []);
      }
      const listeners = this.events.get(event);
      if (listeners.length >= (listeners.maxListeners || Infinity)) {
        console.warn(`Max listeners exceeded for event '${event}'`);
        return;
      }
  
      const wrapper = (data) => {
        if (limit > 0) {
          callback(data);
          limit--;
        }
        if (limit === 0) {
          this.off(event, wrapper);
        }
      };
      this.on(event, wrapper, priority);
    }
  }
  
  // Example usage:
  const emitter = new EventEmitter();
  
  // Regular listener
  emitter.on('data', (data) => console.log('Data received:', data), 1);
  
  // Priority listener
  emitter.on('data', (data) => console.log('High priority data:', data), 10);
  
  // Once listener
  emitter.once('data', (data) => console.log('Once listener:', data));
  
  // Async listener
  emitter.on('asyncData', async (data) => {
    await new Promise(resolve => setTimeout(resolve, 1000));
    console.log('Async data processed:', data);
  });
  
  // Emit events
  emitter.emit('data', { value: 42 });
  emitter.emitAsync('asyncData', { asyncValue: 99 });
  