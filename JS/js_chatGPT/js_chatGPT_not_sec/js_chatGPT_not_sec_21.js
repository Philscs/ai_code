class CircuitBreaker {
    constructor({ failureThreshold = 3, recoveryTimeout = 5000, fallback = null, onMetricsUpdate = null }) {
        this.failureThreshold = failureThreshold;
        this.recoveryTimeout = recoveryTimeout;
        this.fallback = fallback;
        this.onMetricsUpdate = onMetricsUpdate;

        this.failures = 0;
        this.state = 'CLOSED'; // Possible states: 'CLOSED', 'OPEN', 'HALF-OPEN'
        this.lastFailureTime = null;
        this.successCount = 0;
        this.failureCount = 0;
    }

    async call(apiCall) {
        if (this.state === 'OPEN') {
            if (Date.now() - this.lastFailureTime > this.recoveryTimeout) {
                this.state = 'HALF-OPEN';
            } else {
                return this.executeFallback('Circuit is open');
            }
        }

        try {
            const result = await apiCall();
            this.recordSuccess();
            return result;
        } catch (error) {
            this.recordFailure();
            if (this.state === 'HALF-OPEN') {
                this.state = 'OPEN';
            }
            return this.executeFallback(error);
        }
    }

    recordSuccess() {
        this.successCount++;
        this.failures = 0;
        if (this.state === 'HALF-OPEN') {
            this.state = 'CLOSED';
        }
        this.updateMetrics();
    }

    recordFailure() {
        this.failureCount++;
        this.failures++;
        this.lastFailureTime = Date.now();
        if (this.failures >= this.failureThreshold) {
            this.state = 'OPEN';
        }
        this.updateMetrics();
    }

    executeFallback(error) {
        if (this.fallback) {
            return this.fallback(error);
        } else {
            throw error;
        }
    }

    updateMetrics() {
        if (this.onMetricsUpdate) {
            this.onMetricsUpdate({
                successCount: this.successCount,
                failureCount: this.failureCount,
                state: this.state
            });
        }
    }

    healthCheck() {
        return {
            state: this.state,
            successCount: this.successCount,
            failureCount: this.failureCount,
        };
    }
}

// Beispielverwendung
(async () => {
    const breaker = new CircuitBreaker({
        failureThreshold: 3,
        recoveryTimeout: 10000,
        fallback: (error) => ({ error: `Fallback: ${error}` }),
        onMetricsUpdate: (metrics) => console.log('Metrics:', metrics),
    });

    const apiCall = async () => {
        // Simuliere eine API, die manchmal fehlschlägt
        if (Math.random() > 0.7) {
            throw new Error('API Fehler');
        }
        return 'API Erfolg';
    };

    for (let i = 0; i < 10; i++) {
        try {
            const result = await breaker.call(apiCall);
            console.log('Result:', result);
        } catch (error) {
            console.error('Error:', error);
        }

        // Warte ein wenig zwischen den Aufrufen
        await new Promise((resolve) => setTimeout(resolve, 1000));
    }
})();
