class DependencyInjectionContainer {
    constructor() {
      this.services = new Map();
      this.singletons = new Map();
      this.scopes = new Map();
    }
  
    register(name, definition, options = {}) {
      const { lifecycle = 'transient', dependencies = [] } = options;
  
      if (this.services.has(name)) {
        throw new Error(`Service '${name}' is already registered.`);
      }
  
      this.services.set(name, { definition, lifecycle, dependencies });
    }
  
    resolve(name, scopeId = null, visited = new Set()) {
      if (visited.has(name)) {
        throw new Error(`Circular dependency detected: ${Array.from(visited).join(' -> ')} -> ${name}`);
      }
  
      visited.add(name);
  
      const service = this.services.get(name);
      if (!service) {
        throw new Error(`Service '${name}' not found.`);
      }
  
      const { definition, lifecycle, dependencies } = service;
  
      if (lifecycle === 'singleton' && this.singletons.has(name)) {
        return this.singletons.get(name);
      }
  
      if (lifecycle === 'scoped' && scopeId && this.scopes.has(scopeId) && this.scopes.get(scopeId).has(name)) {
        return this.scopes.get(scopeId).get(name);
      }
  
      const dependencyInstances = dependencies.map(dep => this.resolve(dep, scopeId, new Set(visited)));
      const instance = typeof definition === 'function' ? new definition(...dependencyInstances) : definition;
  
      if (lifecycle === 'singleton') {
        this.singletons.set(name, instance);
      } else if (lifecycle === 'scoped' && scopeId) {
        if (!this.scopes.has(scopeId)) {
          this.scopes.set(scopeId, new Map());
        }
        this.scopes.get(scopeId).set(name, instance);
      }
  
      visited.delete(name);
      return instance;
    }
  
    beginScope(scopeId) {
      if (this.scopes.has(scopeId)) {
        throw new Error(`Scope '${scopeId}' already exists.`);
      }
      this.scopes.set(scopeId, new Map());
    }
  
    endScope(scopeId) {
      if (!this.scopes.has(scopeId)) {
        throw new Error(`Scope '${scopeId}' does not exist.`);
      }
      this.scopes.delete(scopeId);
    }
  
    injectProperties(instance, properties) {
      Object.keys(properties).forEach(key => {
        instance[key] = this.resolve(properties[key]);
      });
    }
  }
  
  // Example Usage
  class Logger {
    log(message) {
      console.log(`[LOG]: ${message}`);
    }
  }
  
  class ServiceA {
    constructor(logger) {
      this.logger = logger;
    }
  
    doWork() {
      this.logger.log('ServiceA is working');
    }
  }
  
  class ServiceB {
    constructor(serviceA) {
      this.serviceA = serviceA;
    }
  
    execute() {
      this.serviceA.doWork();
    }
  }
  
  const diContainer = new DependencyInjectionContainer();
  
  diContainer.register('logger', Logger, { lifecycle: 'singleton' });
  diContainer.register('serviceA', ServiceA, { dependencies: ['logger'], lifecycle: 'scoped' });
  diContainer.register('serviceB', ServiceB, { dependencies: ['serviceA'] });
  
  // Scoped resolution
  const scopeId = 'request1';
  diContainer.beginScope(scopeId);
  const serviceB = diContainer.resolve('serviceB', scopeId);
  serviceB.execute();
  diContainer.endScope(scopeId);
  