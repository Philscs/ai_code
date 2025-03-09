// Dienstdefinierung
class LoggerService {
    log(message) {
      console.log(`[${new Date().toISOString()}] ${message}`);
    }
  }
  
  // Service-Implementierung
  class loggerLoggerService implements LoggerService {
    log(message) {
      // Implementierung der Log-Funktion
    }
  }
  
  // Container-Klasse
  class Container {
    constructor() {
      this.services = {};
    }
  
    // Constructor-Injektion
    registerConstructor(constructor: any, ...args: any[]) {
      const instance = new constructor(...args);
      this.services[constructor.name] = instance;
    }
  
    // Property-Injektion
    registerProperty(type: string, value) {
      if (!this.services[type]) {
        throw new Error(`Service '${type}' nicht gefunden`);
      }
      this.services[type].value = value;
    }
  
    // Lifecycle-Management (in diesem Fall einfach ein Event-Listener)
    addLifecycleHandler(type, handler) {
      if (!this.services[type]) {
        throw new Error(`Service '${type}' nicht gefunden`);
      }
      this.services[type].onLoad = handler;
    }
  
    // Scope-Management
    registerScope(type, scope: string) {
      if (scope !== 'global' && scope !== 'local') {
        throw new Error(`Ungültiger Scope '${scope}'.`);
      }
      this.services[type].scope = scope;
    }
  
    // Circular Dependency Detection
    detectCircularDependencies() {
      const visited = {};
      const currentPath = [];
      return Object.keys(this.services).reduce((acc, type) => {
        if (visited[type]) {
          acc.push(type);
          return acc;
        }
        visited[type] = true;
        currentPath.push(type);
        this.checkForCircularDependencies(currentPath);
        return acc;
      }, []);
    }
  
    // Private Methode zur Überprüfung von Circular Dependencies
    checkForCircularDependencies(currentPath) {
      const nextPath = [];
      for (const type of currentPath) {
        if (!currentPath.includes(this.services[type].dependencies)) {
          this.checkForCircularDependencies([...nextPath, ...this.services[type].dependencies]);
        }
      }
    }
  
    // Method zur Auflösung der Abhängigkeiten
    destroy(type) {
      if (type in this.services) {
        delete this.services[type];
      }
    }
  }
  
  // Beispiel-Einrichtung
  const container = new Container();
  
  // Dienstregister
  container.registerConstructor(loggerLoggerService);
  
  // Property-Injektion
  container.registerProperty('logger', 'global');
  
  // Lifecycle-Handler
  container.addLifecycleHandler('logger', () => {
    console.log(`Logger gestartet`);
  });
  
  // Scope-Registrierung
  container.registerScope('logger', 'global');
  