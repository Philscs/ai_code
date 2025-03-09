class EventEmitter {
    constructor() {
      // Store events and their callbacks with priorities
      this.events = new Map();
      // Store single-use event listeners
      this.onceEvents = new Map();
      // Track event listener count for memory leak prevention
      this.listenerCount = new Map();
      // Maximum listeners per event (for memory leak prevention)
      this.maxListeners = 10;
    }
  
    /**
     * Add an event listener with optional priority
     * @param {string} event - Event name
     * @param {Function} callback - Callback function
     * @param {number} [priority=0] - Priority level (higher number = higher priority)
     */
    on(event, callback, priority = 0) {
      this._checkListenerLimit(event);
  
      if (!this.events.has(event)) {
        this.events.set(event, []);
        this.listenerCount.set(event, 0);
      }
  
      const listeners = this.events.get(event);
      const listenerInfo = { callback, priority };
  
      // Insert listener in priority order (highest first)
      const insertIndex = listeners.findIndex(l => l.priority < priority);
      if (insertIndex === -1) {
        listeners.push(listenerInfo);
      } else {
        listeners.splice(insertIndex, 0, listenerInfo);
      }
  
      this.listenerCount.set(event, this.listenerCount.get(event) + 1);
    }
  
    /**
     * Add a one-time event listener with optional priority
     * @param {string} event - Event name
     * @param {Function} callback - Callback function
     * @param {number} [priority=0] - Priority level
     */
    once(event, callback, priority = 0) {
      this._checkListenerLimit(event);
  
      if (!this.onceEvents.has(event)) {
        this.onceEvents.set(event, []);
      }
  
      const listeners = this.onceEvents.get(event);
      const listenerInfo = { callback, priority };
  
      // Insert listener in priority order
      const insertIndex = listeners.findIndex(l => l.priority < priority);
      if (insertIndex === -1) {
        listeners.push(listenerInfo);
      } else {
        listeners.splice(insertIndex, 0, listenerInfo);
      }
  
      this.listenerCount.set(event, (this.listenerCount.get(event) || 0) + 1);
    }
  
    /**
     * Emit an event with data
     * @param {string} event - Event name
     * @param {any} data - Data to pass to listeners
     * @returns {Promise<void>}
     */
    async emit(event, data) {
      const regularListeners = this.events.get(event) || [];
      const onceListeners = this.onceEvents.get(event) || [];
  
      // Combine and sort all listeners by priority
      const allListeners = [...regularListeners, ...onceListeners]
        .sort((a, b) => b.priority - a.priority);
  
      // Execute listeners asynchronously
      try {
        await Promise.all(allListeners.map(async ({ callback }) => {
          try {
            await callback(data);
          } catch (error) {
            console.error(`Error in event listener for ${event}:`, error);
          }
        }));
      } catch (error) {
        console.error(`Error emitting event ${event}:`, error);
      }
  
      // Clean up once listeners
      if (onceListeners.length > 0) {
        this.onceEvents.delete(event);
        this.listenerCount.set(
          event,
          this.listenerCount.get(event) - onceListeners.length
        );
      }
    }
  
    /**
     * Remove an event listener
     * @param {string} event - Event name
     * @param {Function} callback - Callback function to remove
     */
    off(event, callback) {
      if (this.events.has(event)) {
        const listeners = this.events.get(event);
        const index = listeners.findIndex(l => l.callback === callback);
        if (index !== -1) {
          listeners.splice(index, 1);
          this.listenerCount.set(event, this.listenerCount.get(event) - 1);
        }
      }
  
      if (this.onceEvents.has(event)) {
        const listeners = this.onceEvents.get(event);
        const index = listeners.findIndex(l => l.callback === callback);
        if (index !== -1) {
          listeners.splice(index, 1);
          this.listenerCount.set(event, this.listenerCount.get(event) - 1);
        }
      }
    }
  
    /**
     * Set maximum number of listeners per event
     * @param {number} limit - Maximum number of listeners
     */
    setMaxListeners(limit) {
      this.maxListeners = limit;
    }
  
    /**
     * Get current listener count for an event
     * @param {string} event - Event name
     * @returns {number}
     */
    getListenerCount(event) {
      return this.listenerCount.get(event) || 0;
    }
  
    /**
     * Check if adding a new listener would exceed the limit
     * @private
     * @param {string} event - Event name
     */
    _checkListenerLimit(event) {
      const currentCount = this.listenerCount.get(event) || 0;
      if (currentCount >= this.maxListeners) {
        console.warn(
          `Warning: Event '${event}' has exceeded maximum listener limit (${this.maxListeners}).`,
          'This might indicate a memory leak in your application.'
        );
      }
    }
  }
  
  // Usage example:
  const emitter = new EventEmitter();
  
  // Regular listener with priority
  emitter.on('test', async (data) => {
    console.log('Regular listener 1:', data);
  }, 2);
  
  // One-time listener with lower priority
  emitter.once('test', async (data) => {
    console.log('Once listener:', data);
  }, 1);
  
  // Regular listener with lowest priority
  emitter.on('test', async (data) => {
    console.log('Regular listener 2:', data);
  }, 0);
  
  // Emit event
  await emitter.emit('test', { message: 'Hello World' });