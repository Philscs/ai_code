
class EventEmitter {
  constructor() {
    this.events = {};
  }

  on(event, callback) {
    if (!this.events[event]) {
      this.events[event] = [];
    }
    this.events[event].push(callback);
  }

  emit(event, data) {
    const eventCallbacks = this.events[event];
    if (eventCallbacks) {
      eventCallbacks.forEach(callback => {
        callback(data);
      });
    }
  }

  once(event, callback) {
    const onceCallback = (data) => {
      callback(data);
      this.off(event, onceCallback);
    };
    this.on(event, onceCallback);
  }

  off(event, callback) {
    const eventCallbacks = this.events[event];
    if (eventCallbacks) {
      this.events[event] = eventCallbacks.filter(cb => cb !== callback);
    }
  }
}
