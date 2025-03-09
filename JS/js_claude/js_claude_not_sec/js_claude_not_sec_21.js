// Circuit Breaker States
const State = {
    CLOSED: 'CLOSED',
    OPEN: 'OPEN',
    HALF_OPEN: 'HALF_OPEN'
  };
  
  class CircuitBreakerMetrics {
    constructor() {
      this.totalCalls = 0;
      this.failedCalls = 0;
      this.successfulCalls = 0;
      this.lastFailureTime = null;
      this.lastSuccessTime = null;
      this.consecutiveFailures = 0;
    }
  
    recordSuccess() {
      this.totalCalls++;
      this.successfulCalls++;
      this.consecutiveFailures = 0;
      this.lastSuccessTime = Date.now();
    }
  
    recordFailure() {
      this.totalCalls++;
      this.failedCalls++;
      this.consecutiveFailures++;
      this.lastFailureTime = Date.now();
    }
  
    reset() {
      this.consecutiveFailures = 0;
    }
  
    getFailureRate() {
      return this.totalCalls === 0 ? 0 : this.failedCalls / this.totalCalls;
    }
  
    getMetrics() {
      return {
        totalCalls: this.totalCalls,
        failedCalls: this.failedCalls,
        successfulCalls: this.successfulCalls,
        failureRate: this.getFailureRate(),
        lastFailureTime: this.lastFailureTime,
        lastSuccessTime: this.lastSuccessTime,
        consecutiveFailures: this.consecutiveFailures
      };
    }
  }
  
  class CircuitBreaker {
    constructor({
      failureThreshold = 5,
      recoveryTimeout = 10000,
      healthCheckInterval = 5000,
      fallbackFunction = null
    } = {}) {
      this.state = State.CLOSED;
      this.failureThreshold = failureThreshold;
      this.recoveryTimeout = recoveryTimeout;
      this.healthCheckInterval = healthCheckInterval;
      this.fallbackFunction = fallbackFunction;
      this.metrics = new CircuitBreakerMetrics();
      this.lastStateChange = Date.now();
      
      // Start health check monitoring
      this.startHealthCheck();
    }
  
    async execute(apiCall) {
      try {
        switch (this.state) {
          case State.OPEN:
            return await this.handleOpenState(apiCall);
          case State.HALF_OPEN:
            return await this.handleHalfOpenState(apiCall);
          case State.CLOSED:
            return await this.handleClosedState(apiCall);
          default:
            throw new Error('Unknown circuit breaker state');
        }
      } catch (error) {
        throw error;
      }
    }
  
    async handleOpenState(apiCall) {
      const timeInOpen = Date.now() - this.lastStateChange;
      
      if (timeInOpen >= this.recoveryTimeout) {
        this.transitionToHalfOpen();
        return this.execute(apiCall);
      }
  
      return this.handleFailure(new Error('Circuit breaker is OPEN'));
    }
  
    async handleHalfOpenState(apiCall) {
      try {
        const result = await apiCall();
        this.onSuccess();
        return result;
      } catch (error) {
        this.onFailure();
        throw error;
      }
    }
  
    async handleClosedState(apiCall) {
      try {
        const result = await apiCall();
        this.onSuccess();
        return result;
      } catch (error) {
        this.onFailure();
        throw error;
      }
    }
  
    onSuccess() {
      this.metrics.recordSuccess();
      
      if (this.state === State.HALF_OPEN) {
        this.transitionToClosed();
      }
    }
  
    onFailure() {
      this.metrics.recordFailure();
      
      if (this.metrics.consecutiveFailures >= this.failureThreshold) {
        this.transitionToOpen();
      }
    }
  
    transitionToOpen() {
      this.state = State.OPEN;
      this.lastStateChange = Date.now();
      this.emit('stateChange', { from: State.CLOSED, to: State.OPEN });
    }
  
    transitionToHalfOpen() {
      this.state = State.HALF_OPEN;
      this.lastStateChange = Date.now();
      this.emit('stateChange', { from: State.OPEN, to: State.HALF_OPEN });
    }
  
    transitionToClosed() {
      this.state = State.CLOSED;
      this.lastStateChange = Date.now();
      this.metrics.reset();
      this.emit('stateChange', { from: State.HALF_OPEN, to: State.CLOSED });
    }
  
    handleFailure(error) {
      if (this.fallbackFunction) {
        return this.fallbackFunction(error);
      }
      throw error;
    }
  
    startHealthCheck() {
      setInterval(() => {
        this.performHealthCheck();
      }, this.healthCheckInterval);
    }
  
    async performHealthCheck() {
      // Implement your health check logic here
      // For example, you could make a lightweight HTTP request
      try {
        // Perform health check
        // If successful and circuit is open, consider transitioning to half-open
        if (this.state === State.OPEN) {
          const timeInOpen = Date.now() - this.lastStateChange;
          if (timeInOpen >= this.recoveryTimeout) {
            this.transitionToHalfOpen();
          }
        }
      } catch (error) {
        // Health check failed
        if (this.state === State.HALF_OPEN) {
          this.transitionToOpen();
        }
      }
    }
  
    getMetrics() {
      return {
        ...this.metrics.getMetrics(),
        currentState: this.state,
        lastStateChange: this.lastStateChange
      };
    }
  
    // Simple event emitter implementation
    emit(eventName, data) {
      // Implement your event emission logic here
      console.log(`Circuit Breaker Event: ${eventName}`, data);
    }
  }
  
  // Usage example:
  async function example() {
    const circuitBreaker = new CircuitBreaker({
      failureThreshold: 3,
      recoveryTimeout: 5000,
      healthCheckInterval: 3000,
      fallbackFunction: (error) => ({ error: 'Fallback response' })
    });
  
    // Example API call
    const apiCall = async () => {
      const response = await fetch('https://api.example.com/data');
      if (!response.ok) throw new Error('API call failed');
      return response.json();
    };
  
    try {
      const result = await circuitBreaker.execute(apiCall);
      console.log('API call result:', result);
    } catch (error) {
      console.error('Error:', error);
    }
  
    // Get metrics
    console.log('Circuit Breaker Metrics:', circuitBreaker.getMetrics());
  }