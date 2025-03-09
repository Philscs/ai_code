const { Federation } = require('@mfe/federation');
const { LoadBalancer } = require('@mfe/load-balancer');
const { CircuitBreaker } = require('@mfe/circuit-breaker');
const { SecurityBoundary } = require('@mfe/security-boundary');
const { Monitoring } = require('@mfe/monitoring');

class ServiceMesh {
  constructor() {
    this.federation = new Federation();
    this.loadBalancer = new LoadBalancer();
    this.circuitBreaker = new CircuitBreaker();
    this.securityBoundary = new SecurityBoundary();
    this.monitoring = new Monitoring();
  }

  async registerModule(moduleName, modulePath) {
    await this.federation.registerModule(moduleName, modulePath);
  }

  async loadModule(moduleName) {
    return this.federation.loadModule(moduleName);
  }

  async loadBalancer(moduleName) {
    return this.loadBalancer.loadModule(moduleName);
  }

  async circuitBreaker(moduleName) {
    return this.circuitBreaker.circuitBreaker(moduleName);
  }

  async securityBoundary(moduleName) {
    return this.securityBoundary.securityBoundary(moduleName);
  }

  async monitoring(moduleName) {
    return this.monitoring.monitoring(moduleName);
  }
}

class App {
  constructor() {
    this.serviceMesh = new ServiceMesh();
    this.registerModule('module1', './module1/index.js');
    this.loadModule('module1');
    this.loadBalancer('module1');
    this.circuitBreaker('module1');
    this.securityBoundary('module1');
    this.monitoring('module1');
  }

  async start() {
    await this.serviceMesh.registerModule('module2', './module2/index.js');
    console.log('Module 2 registriert und geladen');
    await this.serviceMesh.loadModule('module2');
    console.log('Module 2 geladen');
    await this.serviceMesh.loadBalancer('module2');
    console.log('Load Balancer aktiviert');
    await this.serviceMesh.circuitBreaker('module2');
    console.log('Circuit Breaking aktiviert');
    await this.serviceMesh.securityBoundary('module2');
    console.log('Security Boundary aktiviert');
    await this.serviceMesh.monitoring('module2');
    console.log('Monitoring aktiviert');
  }
}

async function main() {
  const app = new App();
  await app.start();
}

main();