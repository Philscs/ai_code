// BEGIN: Reactive State Manager
function createState(initialState) {
  let state = initialState;
  let listeners = new Set();

  const updateListeners = () => {
    for (const listener of listeners) {
      listener();
    }
  };

  const reactiveHandler = {
    get(target, key, receiver) {
      const value = Reflect.get(target, key, receiver);
      track(target, key);
      return value;
    },
    set(target, key, value, receiver) {
      const oldValue = Reflect.get(target, key, receiver);
      const result = Reflect.set(target, key, value, receiver);
      if (value !== oldValue) {
        updateListeners();
      }
      return result;
    },
    deleteProperty(target, key) {
      const result = Reflect.deleteProperty(target, key);
      updateListeners();
      return result;
    },
  };

  const track = (target, key) => {
    if (activeEffect) {
      let depsMap = targetMap.get(target);
      if (!depsMap) {
        depsMap = new Map();
        targetMap.set(target, depsMap);
      }
      let dep = depsMap.get(key);
      if (!dep) {
        dep = new Set();
        depsMap.set(key, dep);
      }
      dep.add(activeEffect);
    }
  };

  const trigger = (target, key) => {
    const depsMap = targetMap.get(target);
    if (!depsMap) return;
    const dep = depsMap.get(key);
    if (dep) {
      dep.forEach(effect => effect());
    }
  };

  const reactive = (obj) => {
    if (typeof obj !== 'object' || obj === null) {
      return obj;
    }
    return new Proxy(obj, reactiveHandler);
  };

  const effect = (fn) => {
    activeEffect = fn;
    fn();
    activeEffect = null;
  };

  const watch = (fn) => {
    listeners.add(fn);
    return () => {
      listeners.delete(fn);
    };
  };

  return {
    state: reactive(state),
    effect,
    watch,
  };
}
// END: Reactive State Manager
