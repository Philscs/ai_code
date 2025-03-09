// Main Store class for state management
class Store {
    constructor(reducer, initialState = {}, middlewares = []) {
      this.state = initialState;
      this.reducer = reducer;
      this.listeners = new Set();
      this.middlewares = middlewares;
      this.dependencies = new Set();
      
      // DevTools Integration
      if (typeof window !== 'undefined' && window.__REDUX_DEVTOOLS_EXTENSION__) {
        this.devTools = window.__REDUX_DEVTOOLS_EXTENSION__.connect({
          name: 'Custom Store',
          features: {
            jump: true,
            skip: true,
            pause: true,
            revert: true,
            persist: true
          }
        });
        this.devTools.init(this.state);
      }
  
      // Build middleware chain
      const chain = middlewares.map(middleware => middleware(this));
      this.dispatch = chain.reduceRight(
        (next, middleware) => middleware(next),
        this.baseDispatch.bind(this)
      );
    }
  
    // Getter for current state
    getState() {
      return this.state;
    }
  
    // Base dispatch function
    baseDispatch(action) {
      if (!action || typeof action !== 'object' || !action.type) {
        throw new Error('Actions must be plain objects with a type property');
      }
  
      // Check for circular dependencies
      if (this.checkCircularDependencies(action)) {
        console.error('Circular dependency detected in action:', action);
        return;
      }
  
      // Immutable state update
      const nextState = this.reducer(this.state, action);
      
      // Validate state is not mutated
      if (nextState === this.state) {
        console.warn('Reducer returned same state reference. Ensure you\'re not mutating state.');
      }
  
      this.state = nextState;
      
      // DevTools update
      if (this.devTools) {
        this.devTools.send(action, this.state);
      }
  
      // Notify listeners
      this.listeners.forEach(listener => listener());
  
      return action;
    }
  
    // Subscribe method for reactivity
    subscribe(listener) {
      if (typeof listener !== 'function') {
        throw new Error('Listener must be a function');
      }
      
      this.listeners.add(listener);
      
      // Return unsubscribe function
      return () => {
        this.listeners.delete(listener);
      };
    }
  
    // Helper method to detect circular dependencies
    checkCircularDependencies(action) {
      const actionType = action.type;
      if (this.dependencies.has(actionType)) {
        return true;
      }
      this.dependencies.add(actionType);
      setTimeout(() => {
        this.dependencies.delete(actionType);
      }, 0);
      return false;
    }
  
    // Helper method to create action creators
    static createAction(type) {
      return payload => ({
        type,
        payload
      });
    }
  }
  
  // Example Logger Middleware
  const logger = store => next => action => {
    console.group(action.type);
    console.log('Previous State:', store.getState());
    console.log('Action:', action);
    const result = next(action);
    console.log('Next State:', store.getState());
    console.groupEnd();
    return result;
  };
  
  // Example Thunk Middleware for async actions
  const thunk = store => next => action => {
    if (typeof action === 'function') {
      return action(store.dispatch, store.getState);
    }
    return next(action);
  };
  
  // Example error handling middleware
  const errorHandler = store => next => action => {
    try {
      return next(action);
    } catch (error) {
      console.error('Error in action:', action);
      console.error(error);
      throw error;
    }
  };
  
  // Example Reducer
  const exampleReducer = (state = {}, action) => {
    switch (action.type) {
      case 'SET_DATA':
        return {
          ...state,
          data: action.payload
        };
      case 'UPDATE_DATA':
        return {
          ...state,
          data: {
            ...state.data,
            ...action.payload
          }
        };
      case 'CLEAR_DATA':
        return {
          ...state,
          data: null
        };
      default:
        return state;
    }
  };
  
  // Example usage:
  const store = new Store(
    exampleReducer,
    { data: null },
    [errorHandler, logger, thunk]
  );
  
  // Create action creators
  const setData = Store.createAction('SET_DATA');
  const updateData = Store.createAction('UPDATE_DATA');
  const clearData = Store.createAction('CLEAR_DATA');
  
  // Subscribe example
  const unsubscribe = store.subscribe(() => {
    console.log('State updated:', store.getState());
  });
  
  // Dispatch examples
  store.dispatch(setData({ value: 42 }));
  
  store.dispatch(updateData({ newValue: 43 }));
  
  // Async action example
  store.dispatch(async (dispatch, getState) => {
    try {
      const response = await fetch('https://api.example.com/data');
      const data = await response.json();
      dispatch(setData(data));
    } catch (error) {
      console.error('API Error:', error);
    }
  });
  
  export default Store;