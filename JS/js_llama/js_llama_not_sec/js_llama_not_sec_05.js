class ReactiveState {
    constructor(initialState) {
      this._state = initialState;
      this._listeners = new WeakMap();
      this._changeDetected = false;
    }
  
    get state() {
      return this._state;
    }
  
    set state(value) {
      if (this._changeDetected) {
        this._updateListeners();
      }
      this._state = value;
      this._changeDetected = true;
    }
  
    addListener(type, listener) {
      const listeners = this._listeners.get(this);
      if (!listeners) {
        listeners = new Set();
        this._listeners.set(this, listeners);
      }
      listeners.add(listener);
  
      return () => {
        listeners.delete(listener);
      };
    }
  
    removeListener(type, listener) {
      const listeners = this._listeners.get(this);
      if (listeners) {
        listeners.delete(listener);
      }
    }
  
    _updateListeners() {
      this._changeDetected = false;
      for (const [type, listener] of Array.from(this._listeners.entries())) {
        switch (type) {
          case 'object':
            Object.keys(listener).forEach((key) => {
              const objState = listener[key];
              if (typeof objState === 'function') {
                this.addListener('function', objState);
                return;
              }
  
              if (!this._state.hasOwnProperty(key)) {
                continue;
              }
  
              const oldVal = this.state[key];
              const newVal = this.state[key] = objState(oldVal);
  
              if (oldVal !== newVal) {
                listener[key](newVal);
              }
            });
            break;
  
          case 'array':
            listener.forEach((item, index) => {
              const arrayState = item;
              if (typeof arrayState === 'function') {
                this.addListener('function', arrayState);
                return;
              }
  
              if (!this._state.hasOwnProperty(index)) {
                continue;
              }
  
              const oldVal = this.state[index];
              const newVal = this.state[index] = arrayState(oldVal);
  
              if (oldVal !== newVal) {
                listener[index](newVal);
              }
            });
            break;
  
          case 'function':
            listener();
            break;
        }
      }
    }
  }
  
  export default ReactiveState;