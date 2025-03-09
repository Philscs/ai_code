// BEGIN: State Management System

// Define the initial state
const initialState = {
  counter: 0,
};

// Define the reducer function
function reducer(state = initialState, action) {
  switch (action.type) {
    case 'INCREMENT':
      return {
        ...state,
        counter: state.counter + 1,
      };
    case 'DECREMENT':
      return {
        ...state,
        counter: state.counter - 1,
      };
    default:
      return state;
  }
}

// Create the store
function createStore(reducer, middleware) {
  let state = reducer(undefined, {});
  const subscribers = [];

  function dispatch(action) {
    state = reducer(state, action);
    subscribers.forEach((subscriber) => subscriber());
  }

  function getState() {
    return state;
  }

  function subscribe(subscriber) {
    subscribers.push(subscriber);
    return () => {
      const index = subscribers.indexOf(subscriber);
      if (index !== -1) {
        subscribers.splice(index, 1);
      }
    };
  }

  // Apply middleware
  if (middleware) {
    const middlewareAPI = {
      getState,
      dispatch: (action) => dispatch(action),
    };
    const enhancedDispatch = middleware(middlewareAPI)(dispatch);
    dispatch = enhancedDispatch;
  }

  return {
    dispatch,
    getState,
    subscribe,
  };
}

// Define middleware
function loggerMiddleware({ getState }) {
  return (next) => (action) => {
    console.log('Action:', action);
    console.log('Previous State:', getState());
    const result = next(action);
    console.log('Next State:', getState());
    return result;
  };
}

// Create the store with middleware
const store = createStore(reducer, loggerMiddleware);

// Subscribe to state changes
const unsubscribe = store.subscribe(() => {
  const state = store.getState();
  console.log('Current State:', state);
});

// Dispatch actions
store.dispatch({ type: 'INCREMENT' });
store.dispatch({ type: 'INCREMENT' });
store.dispatch({ type: 'DECREMENT' });

// Unsubscribe from state changes
unsubscribe();

// END: State Management System
