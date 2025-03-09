class ReactiveState {
    constructor(initialState = {}) {
      // Speichert alle Subscriber (Komponenten/Callbacks)
      this.subscribers = new Set();
      
      // Erstellt den reaktiven State mit Proxy
      this.state = this.makeReactive(initialState);
    }
  
    // Macht ein Objekt oder Array reaktiv
    makeReactive(target) {
      // Verhindert doppelte Wrapping
      if (target.__isReactive) {
        return target;
      }
  
      const self = this;
  
      // Erstellt einen Proxy für das Target-Objekt
      return new Proxy(target, {
        get(obj, key) {
          const value = obj[key];
          
          // Macht verschachtelte Objekte/Arrays auch reaktiv
          if (typeof value === 'object' && value !== null) {
            return self.makeReactive(value);
          }
          
          return value;
        },
  
        set(obj, key, value) {
          // Verhindert unnötige Updates bei gleichem Wert
          if (obj[key] === value) {
            return true;
          }
  
          // Setzt den neuen Wert
          obj[key] = value;
  
          // Benachrichtigt alle Subscriber
          self.notify();
          
          return true;
        },
  
        deleteProperty(obj, key) {
          if (key in obj) {
            delete obj[key];
            self.notify();
          }
          return true;
        }
      });
    }
  
    // Registriert einen neuen Subscriber
    subscribe(callback) {
      this.subscribers.add(callback);
      
      // Gibt eine Cleanup-Funktion zurück
      return () => {
        this.subscribers.delete(callback);
      };
    }
  
    // Benachrichtigt alle Subscriber über Änderungen
    notify() {
      this.subscribers.forEach(callback => callback(this.state));
    }
  
    // Getter für den aktuellen State
    getState() {
      return this.state;
    }
  
    // Setter für den kompletten State
    setState(newState) {
      Object.keys(newState).forEach(key => {
        this.state[key] = newState[key];
      });
    }
  }
  
  // Beispiel-Komponente die den State verwendet
  class Component {
    constructor(store) {
      this.store = store;
      this.unsubscribe = this.store.subscribe(this.render.bind(this));
    }
  
    render(state) {
      console.log('Component rendering with state:', state);
    }
  
    destroy() {
      this.unsubscribe();
    }
  }
  
  // Beispiel-Verwendung:
  const store = new ReactiveState({
    user: {
      name: 'Max',
      settings: {
        theme: 'dark'
      }
    },
    todos: []
  });
  
  // Komponente erstellen
  const comp = new Component(store);
  
  // State ändern - löst automatisch Updates aus
  store.state.user.name = 'Maria';
  store.state.user.settings.theme = 'light';
  store.state.todos.push({ text: 'Learn ReactiveState', done: false });
  
  // Mehrere Änderungen auf einmal
  store.setState({
    user: {
      name: 'John',
      settings: { theme: 'system' }
    }
  });