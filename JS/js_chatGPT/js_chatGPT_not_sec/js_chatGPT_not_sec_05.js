// Reactive State Manager mit Proxy-basiertem Change Detection
class ReactiveState {
    constructor(initialState) {
      this.listeners = new Map();
      this.state = this._makeReactive(initialState);
    }
  
    // Konvertiert ein Objekt oder Array in ein reaktives Proxy-Objekt
    _makeReactive(target, path = []) {
      const self = this;
  
      return new Proxy(target, {
        get(obj, key) {
          const value = obj[key];
          if (typeof value === 'object' && value !== null) {
            return self._makeReactive(value, path.concat(key));
          }
          return value;
        },
        set(obj, key, value) {
          const oldValue = obj[key];
          obj[key] = value;
  
          if (oldValue !== value) {
            self._notifyListeners(path.concat(key), value, oldValue);
          }
          return true;
        },
        deleteProperty(obj, key) {
          if (key in obj) {
            const oldValue = obj[key];
            delete obj[key];
            self._notifyListeners(path.concat(key), undefined, oldValue);
          }
          return true;
        }
      });
    }
  
    // Fügt eine Listener-Funktion hinzu, die bei Änderungen aufgerufen wird
    addListener(keyPath, callback) {
      const key = keyPath.join('.');
      if (!this.listeners.has(key)) {
        this.listeners.set(key, []);
      }
      this.listeners.get(key).push(callback);
    }
  
    // Entfernt eine Listener-Funktion
    removeListener(keyPath, callback) {
      const key = keyPath.join('.');
      if (this.listeners.has(key)) {
        const callbacks = this.listeners.get(key);
        this.listeners.set(key, callbacks.filter(cb => cb !== callback));
      }
    }
  
    // Benachrichtigt Listener bei Änderungen
    _notifyListeners(path, newValue, oldValue) {
      const key = path.join('.');
  
      // Notify exact listeners
      if (this.listeners.has(key)) {
        this.listeners.get(key).forEach(callback => callback(newValue, oldValue));
      }
  
      // Notify listeners for parent keys
      let parentPath = path.slice(0, -1);
      while (parentPath.length) {
        const parentKey = parentPath.join('.');
        if (this.listeners.has(parentKey)) {
          this.listeners.get(parentKey).forEach(callback => callback(this.state, oldValue));
        }
        parentPath.pop();
      }
    }
  
    // Zugriff auf den State
    getState() {
      return this.state;
    }
  }
  
  // Beispielnutzung
  const state = new ReactiveState({
    user: {
      name: 'Alex',
      hobbies: ['Reading', 'Gaming']
    }
  });
  
  // Listener hinzufügen
  state.addListener(['user', 'name'], (newVal, oldVal) => {
    console.log(`Name geändert von ${oldVal} zu ${newVal}`);
  });
  
  state.addListener(['user', 'hobbies'], (newVal) => {
    console.log('Hobbies geändert:', newVal);
  });
  
  // Reaktive Änderungen vornehmen
  state.getState().user.name = 'Taylor'; // Name geändert von Alex zu Taylor
  state.getState().user.hobbies.push('Cooking'); // Hobbies geändert: ['Reading', 'Gaming', 'Cooking']
  