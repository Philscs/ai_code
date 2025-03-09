
class WebPerformanceMonitoringSystem {
  constructor() {
    this.metrics = [];
    this.users = [];
    this.securityCompliance = false;
    this.privacyControls = false;
    this.resourceUsage = {};
  }

  collectMetrics(metric) {
    this.metrics.push(metric);
  }

  monitorUser(user) {
    this.users.push(user);
  }

  enableSecurityCompliance() {
    this.securityCompliance = true;
  }

  disableSecurityCompliance() {
    this.securityCompliance = false;
  }

  enablePrivacyControls() {
    this.privacyControls = true;
  }

  disablePrivacyControls() {
    this.privacyControls = false;
  }

  updateResourceUsage(resource, usage) {
    this.resourceUsage[resource] = usage;
  }
}

// Usage example:
const monitoringSystem = new WebPerformanceMonitoringSystem();

monitoringSystem.collectMetrics("Page load time");
monitoringSystem.collectMetrics("Network latency");

monitoringSystem.monitorUser("John Doe");
monitoringSystem.monitorUser("Jane Smith");

monitoringSystem.enableSecurityCompliance();
monitoringSystem.enablePrivacyControls();

monitoringSystem.updateResourceUsage("CPU", 80);
monitoringSystem.updateResourceUsage("Memory", 60);

console.log(monitoringSystem);
