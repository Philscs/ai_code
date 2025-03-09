class CircuitBreaker {
    constructor(failureThreshold, recoveryTimeout, fallbackStrategy, metricsCollector) {
      this.failureThreshold = failureThreshold;
      this.recoveryTimeout = recoveryTimeout;
      this.fallbackStrategy = fallbackStrategy;
      this.metricsCollector = metricsCollector;
      this.isOpen = false;
      this.openedAt = null;
    }
  
    async callApi(url, options) {
      if (this.isOpen) {
        return await this.fallbackStrategy();
      }
  
      let failedCount = 0;
      const startTime = new Date().getTime();
  
      for (let attempt = 1; attempt <= this.failureThreshold; attempt++) {
        try {
          const response = await fetch(url, options);
          if (!response.ok) {
            throw new Error(`API Call to ${url} failed with status code ${response.status}`);
          }
          return await response.json();
        } catch (error) {
          console.error(error);
          failedCount++;
          if (failedCount >= this.failureThreshold) {
            this.isOpen = true;
            this.openedAt = new Date().getTime();
            break;
          }
        }
      }
  
      const duration = new Date().getTime() - startTime;
      this.metricsCollector.collect('api_call_duration', duration);
      return await this.fallbackStrategy();
    }
  
    async recover() {
      if (this.isOpen) {
        this.isOpen = false;
        setTimeout(() => {
          console.log('Circuit breaker has recovered');
        }, this.recoveryTimeout);
      }
    }
  
    async healthCheck() {
      const response = await fetch('https://example.com/healthcheck', { method: 'GET' });
      if (!response.ok) {
        return false;
      }
      return true;
    }
  }
  
  class MetricsCollector {
    collect(key, value) {
      console.log(`Metric collected: ${key}=${value}`);
    }
  }
  