// State Management System
class Store {
    constructor(reducer, initialState, middleware = []) {
      this.state = initialState;
      this.reducer = reducer;
      this.middleware = middleware;
      this.listeners = [];
      this.dispatch = this.createDispatch();
      this.devTools = this.initDevTools();
    }
  
    // Immutable State Getter
    getState() {
      return Object.freeze({ ...this.state });
    }
  
    // Dispatch with Middleware
    createDispatch() {
      const baseDispatch = (action) => {
        const newState = this.reducer(this.state, action);
        if (this.state === newState) return; // Prevent circular dependencies
        this.state = newState;
        this.notify();
        if (this.devTools) this.devTools.send(action, this.state);
      };
  
      const chain = this.middleware.map((mw) => mw({ getState: () => this.state, dispatch: baseDispatch }));
      return chain.reduce((acc, mw) => mw(acc), baseDispatch);
    }
  
    // Subscribe to State Changes
    subscribe(listener) {
      this.listeners.push(listener);
      return () => {
        this.listeners = this.listeners.filter((l) => l !== listener);
      };
    }
  
    // Notify Subscribers
    notify() {
      this.listeners.forEach((listener) => listener(this.getState()));
    }
  
    // DevTools Integration
    initDevTools() {
      if (typeof window !== 'undefined' && window.__REDUX_DEVTOOLS_EXTENSION__) {
        return window.__REDUX_DEVTOOLS_EXTENSION__.connect({
          name: 'CustomStore',
        });
      }
      return null;
    }
  }
  
  // Action Types with TypeScript-Like Typing
  /**
   * @template S, A
   * @typedef {(state: S, action: A) => S} Reducer
   */
  
  // Example Reducer
  const initialState = { counter: 0 };
  const reducer = (state = initialState, action) => {
    switch (action.type) {
      case 'INCREMENT':
        return { ...state, counter: state.counter + 1 };
      case 'DECREMENT':
        return { ...state, counter: state.counter - 1 };
      default:
        return state;
    }
  };
  
  // Middleware Example
  const loggerMiddleware = ({ getState }) => (next) => (action) => {
    console.log('Dispatching:', action);
    next(action);
    console.log('Next State:', getState());
  };
  
  const circularDependencyPreventionMiddleware = ({ getState }) => (next) => (action) => {
    const prevState = getState();
    next(action);
    const nextState = getState();
    if (prevState === nextState) {
      console.warn('Circular dependency detected! State did not change.');
    }
  };
  
  // Create Store
  const store = new Store(reducer, initialState, [loggerMiddleware, circularDependencyPreventionMiddleware]);
  
  // Usage
  store.subscribe((state) => console.log('State Updated:', state));
  store.dispatch({ type: 'INCREMENT' });
  store.dispatch({ type: 'DECREMENT' });
  