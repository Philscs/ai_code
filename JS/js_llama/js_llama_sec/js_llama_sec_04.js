import { Action } from './actions';
import { LoggingMiddleware } from './middleware';
import { Reducer } from './reducer';

class Store {
  private state = {};
  private middlewareQueue = [];

  constructor(options) {
    this.state = options.initialState || {};
    this.middlewareQueue.push(LoggingMiddleware);

    if (options.middlewares) {
      this.middlewareQueue.push(...options.middlewares);
    }

    for (let i = 0; i < this.middlewareQueue.length; i++) {
      const middleware = this.middlewareQueue[i];

      if (!middleware.name || middleware.name === 'CircularDependencyMiddleware') {
        continue;
      }

      try {
        this.middlewareQueue[i] = middleware.handle.bind(middleware, this.state);
      } catch (error) {
        console.error(error);
        throw error;
      }
    }

    const Reducer = (state, action) => {
      switch (action.type) {
        case 'ADD_ITEM':
          return { ...state, items: [...state.items, action.data] };
        case 'REMOVE_ITEM':
          return { ...state, items: state.items.filter((item) => item.key !== action.data.key) };
        default:
          return state;
      }
    };

    this.middlewareQueue.forEach((middleware) => {
      try {
        const nextState = middleware(this.middlewareQueue[i + 1], this.state);
        if (nextState !== this.state) {
          this.state = nextState;
        }
      } catch (error) {
        console.error(error);
        throw error;
      }
    });
  }

  public dispatch(action) {
    const index = this.middlewareQueue.findIndex((item) => item.name === 'LoggingMiddleware');
    if (index !== -1) {
      try {
        return this.middlewareQueue[index].handle(action, this.state);
      } catch (error) {
        console.error(error);
        throw error;
      }
    }

    let nextState = this.state;

    for (let i = 0; i < this.middlewareQueue.length; i++) {
      const middleware = this.middlewareQueue[i];

      if (!middleware.name || middleware.name === 'CircularDependencyMiddleware') {
        continue;
      }

      try {
        nextState = middleware(this.middlewareQueue[i + 1], nextState);
      } catch (error) {
        console.error(error);
        throw error;
      }
    }

    return Reducer.reduce(this.state, action);
  }

  public getState() {
    return this.state;
  }

  public addMiddleware(middleware) {
    this.middlewareQueue.push(middleware);
  }
}

class StateManagementSystem {
  private store;

  constructor(options = {}) {
    this.store = new Store(options);
  }

  public dispatch(action) {
    return this.store.dispatch(action);
  }

  public getState() {
    return this.store.getState();
  }

  public addMiddleware(middleware) {
    this.store.addMiddleware(middleware);
  }
}

export { StateManagementSystem, Store };
class LoggingMiddleware {
  static readonly name = 'LoggingMiddleware';

  public async handle(action, state) {
    console.log(`Action received: ${action.type}, Data: ${JSON.stringify(action.data)}`);
    return action.data;
  }
}

class CircularDependencyMiddleware {
  static readonly name = 'CircularDependencyMiddleware';

  public async handle(_, state) {
    throw new Error('Circular dependency detected');
  }
}
const App = () => {
  const dispatch = (action) => StateManagementSystem.dispatch(action);

  return (
    <div>
      <button onClick={() => dispatch(new Action('ADD_ITEM', { key: 'item1', value: 'value1' 
}))}>
        Add Item
      </button>

      <button onClick={() => dispatch(new Action('REMOVE_ITEM', { key: 'item1' }))}>
        Remove Item
      </button>

      <button onClick={() => StateManagementSystem.addMiddleware(CircularDependencyMiddleware)}>
        Add Circular Dependency Middleware
      </button>
    </div>
  );
};

const actions = {
  ADD_ITEM: (data) => ({ type: 'ADD_ITEM', data }),
  REMOVE_ITEM: (key) => ({ type: 'REMOVE_ITEM', key }),
};

export { App, actions };