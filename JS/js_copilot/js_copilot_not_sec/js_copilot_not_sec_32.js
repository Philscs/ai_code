class Container {
  constructor() {
    this.services = new Map();
    this.scopes = new Map();
  }

  register(name, service, dependencies = []) {
    this.services.set(name, { service, dependencies });
  }

  resolve(name) {
    if (!this.services.has(name)) {
      throw new Error(`Service '${name}' not found.`);
    }

    const { service, dependencies } = this.services.get(name);

    if (this.scopes.has(name)) {
      return this.scopes.get(name);
    }

    const resolvedDependencies = dependencies.map(dep => this.resolve(dep));
    const instance = new service(...resolvedDependencies);

    this.scopes.set(name, instance);

    return instance;
  }

  dispose() {
    this.scopes.clear();
  }
}

class ServiceA {
  constructor(serviceB) {
    this.serviceB = serviceB;
  }

  doSomething() {
    console.log("Service A is doing something.");
    this.serviceB.doSomethingElse();
  }
}

class ServiceB {
  constructor(serviceA) {
    this.serviceA = serviceA;
  }

  doSomethingElse() {
    console.log("Service B is doing something else.");
    this.serviceA.doSomething();
  }
}

// Usage example
const container = new Container();

container.register("serviceA", ServiceA, ["serviceB"]);
container.register("serviceB", ServiceB, ["serviceA"]);

const serviceA = container.resolve("serviceA");
serviceA.doSomething();

container.dispose();
