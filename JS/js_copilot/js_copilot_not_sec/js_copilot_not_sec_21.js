class CircuitBreaker {
  constructor(failureThreshold, recoveryTimeout, fallbackStrategy) {
    this.failureThreshold = failureThreshold;
    this.recoveryTimeout = recoveryTimeout;
    this.fallbackStrategy = fallbackStrategy;
    this.failureCount = 0;
    this.lastFailureTime = null;
    this.isOpen = false;
  }

  async callApi(apiCall) {
    if (this.isOpen && this.isRecoveryTimeoutExpired()) {
      this.reset();
    }

    if (this.isOpen) {
      return this.fallbackStrategy();
    }

    try {
      const response = await apiCall();
      this.handleSuccess();
      return response;
    } catch (error) {
      this.handleFailure();
      return this.fallbackStrategy();
    }
  }

  handleSuccess() {
    this.failureCount = 0;
    this.lastFailureTime = null;
  }

  handleFailure() {
    this.failureCount++;
    this.lastFailureTime = new Date();

    if (this.failureCount >= this.failureThreshold) {
      this.isOpen = true;
      setTimeout(() => {
        this.reset();
      }, this.recoveryTimeout);
    }
  }

  reset() {
    this.failureCount = 0;
    this.lastFailureTime = null;
    this.isOpen = false;
  }

  isRecoveryTimeoutExpired() {
    const currentTime = new Date();
    const elapsedTime = currentTime - this.lastFailureTime;
    return elapsedTime >= this.recoveryTimeout;
  }
}

// Example usage
const circuitBreaker = new CircuitBreaker(3, 5000, fallbackStrategy);

async function apiCall() {
  // Make API call here
}

async function fallbackStrategy() {
  // Fallback logic here
}

circuitBreaker.callApi(apiCall)
  .then(response => {
    // Handle API response
  })
  .catch(error => {
    // Handle error
  });
