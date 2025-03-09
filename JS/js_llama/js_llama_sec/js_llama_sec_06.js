function EventEmitter() {
    this.listeners = {};
  }
  
  EventEmitter.prototype.on = function(event, callback) {
    if (!this.listeners[event]) {
      this.listeners[event] = [];
    }
    this.listeners[event].push(callback);
  };
  
  EventEmitter.prototype.emit = function(event, data) {
    if (!this.listeners[event]) {
      throw new Error(`Kein Listener für das Ereignis '${event}' gefunden.`);
    }
    const listeners = this.listeners[event];
    for (let i = 0; i < listeners.length; i++) {
      listeners[i](data);
    }
  };
  
  EventEmitter.prototype.once = function(event, callback) {
    this.on(event, callback);
    return function() {
      this.off(event, callback);
    }.bind(this);
  };
  
  EventEmitter.prototype.off = function(event, callback) {
    if (!this.listeners[event]) {
      return;
    }
    const listeners = this.listeners[event];
    const index = listeners.indexOf(callback);
    if (index !== -1) {
      listeners.splice(index, 1);
    } else {
      delete this.listeners[event];
    }
  };
  
  EventEmitter.prototype.offAll = function(event) {
    if (this.listeners[event]) {
      delete this.listeners[event];
    }
  };
  
  EventEmitter.prototype.syncEmit = function(event, data) {
    const listeners = this.listeners[event];
    for (let i = 0; i < listeners.length; i++) {
      listeners[i](data);
    }
  };
  
  EventEmitter.prototype asyncEmit = function(event, data) {
    return new Promise((resolve) => {
      const listeners = this.listeners[event];
      if (!listeners) {
        throw new Error(`Kein Listener für das Ereignis '${event}' gefunden.`);
      }
      for (let i = 0; i < listeners.length; i++) {
        listeners[i](data);
      }
      const queue = [];
      function processQueue() {
        while (queue.length > 0 && queue[0][1] === undefined) {
          const [event, data] = queue.shift();
          for (let i = 0; i < this.listeners[event].length; i++) {
            this.listeners[event][i](data);
          }
        }
        if (queue.length > 0) {
          globalThis.setTimeout(processQueue, 0);
        } else {
          resolve();
        }
      }.bind(this);
      processQueue();
    });
  };
  
  // Beispiel für die Verwendung des Event Emitters
  const emitter = new EventEmitter();
  
  emitter.on('test', function(data) {
    globalThis.console.log(`Test mit Daten ${data}`);
  });
  
  emitter.emit('test', 42);
  
  emitter.once('test', function(data) {
    globalThis.console.log(`Test mit Daten ${data}`);
  });
  
  emitter.off('test', function() {
    globalThis.console.log('Listener entfernt');
  });