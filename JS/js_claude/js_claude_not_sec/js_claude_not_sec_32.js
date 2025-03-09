class DIContainer {
    constructor() {
      this.dependencies = new Map();
      this.instances = new Map();
      this.constructing = new Set();
      this.propertyInjections = new Map();
    }
  
    // Registrierung von Dependencies mit verschiedenen Lifecycles
    register(name, definition, options = {}) {
      const {
        scope = 'singleton', // 'singleton' | 'transient' | 'scoped'
        properties = {}      // Für Property Injection
      } = options;
  
      this.dependencies.set(name, { definition, scope });
      
      if (Object.keys(properties).length > 0) {
        this.propertyInjections.set(name, properties);
      }
  
      return this;
    }
  
    // Hauptmethode zum Auflösen von Dependencies
    resolve(name, scope = {}) {
      if (!this.dependencies.has(name)) {
        throw new Error(`Dependency ${name} ist nicht registriert`);
      }
  
      const { definition, scope: lifecycleScope } = this.dependencies.get(name);
  
      // Erkennung zirkulärer Abhängigkeiten
      if (this.constructing.has(name)) {
        throw new Error(
          `Zirkuläre Abhängigkeit erkannt: ${Array.from(this.constructing).join(' -> ')} -> ${name}`
        );
      }
  
      // Lifecycle Management
      switch (lifecycleScope) {
        case 'singleton':
          if (this.instances.has(name)) {
            return this.instances.get(name);
          }
          break;
        case 'scoped':
          if (scope[name]) {
            return scope[name];
          }
          break;
        // Bei 'transient' wird immer eine neue Instanz erstellt
      }
  
      // Constructor Injection
      this.constructing.add(name);
      let instance;
  
      try {
        if (typeof definition === 'function') {
          // Analyse der Constructor-Parameter
          const paramNames = this.getConstructorParams(definition);
          const params = paramNames.map(param => this.resolve(param, scope));
          instance = new definition(...params);
        } else {
          instance = definition;
        }
  
        // Property Injection
        if (this.propertyInjections.has(name)) {
          const properties = this.propertyInjections.get(name);
          for (const [prop, depName] of Object.entries(properties)) {
            instance[prop] = this.resolve(depName, scope);
          }
        }
  
        // Lifecycle Management - Speichern der Instanz
        if (lifecycleScope === 'singleton') {
          this.instances.set(name, instance);
        } else if (lifecycleScope === 'scoped') {
          scope[name] = instance;
        }
  
      } finally {
        this.constructing.delete(name);
      }
  
      return instance;
    }
  
    // Hilfsmethode zur Analyse der Constructor-Parameter
    getConstructorParams(func) {
      const str = func.toString();
      const match = str.match(/constructor\s*\(([\s\S]*?)\)/);
      if (!match) return [];
      
      const params = match[1].split(',')
        .map(param => param.trim())
        .filter(param => param);
      
      return params;
    }
  
    // Erstellt einen neuen Scope für scoped Services
    createScope() {
      return {};
    }
  
    // Cleanup-Methode
    dispose() {
      for (const [name, instance] of this.instances) {
        if (typeof instance.dispose === 'function') {
          instance.dispose();
        }
      }
      this.instances.clear();
    }
  }
  
  // Beispiel zur Verwendung:
  class Database {
    constructor() {
      this.connected = false;
    }
  
    connect() {
      this.connected = true;
    }
  
    dispose() {
      this.connected = false;
    }
  }
  
  class UserService {
    constructor(database) {
      this.database = database;
    }
  }
  
  class Logger {
    log(message) {
      console.log(message);
    }
  }
  
  class UserController {
    constructor(userService) {
      this.userService = userService;
    }
  }
  
  // Verwendung:
  const container = new DIContainer();
  
  // Registrierung der Dependencies
  container
    .register('database', Database, { scope: 'singleton' })
    .register('userService', UserService, { scope: 'scoped' })
    .register('logger', Logger, { scope: 'transient' })
    .register('userController', UserController, {
      scope: 'transient',
      properties: { logger: 'logger' } // Property Injection
    });
  
  // Verwendung mit verschiedenen Scopes
  const scope1 = container.createScope();
  const scope2 = container.createScope();
  
  const userController1 = container.resolve('userController', scope1);
  const userController2 = container.resolve('userController', scope2);
  
  // Cleanup
  container.dispose();