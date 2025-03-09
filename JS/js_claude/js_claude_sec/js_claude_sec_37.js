// Web Performance Monitoring System

class PerformanceMonitor {
    constructor(config = {}) {
      this.config = {
        sampleRate: config.sampleRate || 100, // Percentage of users to monitor
        privacyMode: config.privacyMode || 'strict',
        retentionDays: config.retentionDays || 30,
        ...config
      };
      
      this.metrics = {
        performance: {},
        resources: {},
        errors: []
      };
      
      this.initializeMonitoring();
    }
  
    // Metrics Collection
    initializeMonitoring() {
      this.collectNavigationTimings();
      this.collectResourceTimings();
      this.setupErrorTracking();
      this.setupUserMonitoring();
    }
  
    collectNavigationTimings() {
      const navigation = performance.getEntriesByType('navigation')[0];
      if (navigation) {
        this.metrics.performance.pageLoad = {
          dnsLookup: navigation.domainLookupEnd - navigation.domainLookupStart,
          tcpConnection: navigation.connectEnd - navigation.connectStart,
          serverResponse: navigation.responseEnd - navigation.requestStart,
          domComplete: navigation.domComplete - navigation.responseEnd,
          loadComplete: navigation.loadEventEnd - navigation.loadEventStart
        };
      }
    }
  
    collectResourceTimings() {
      const resources = performance.getEntriesByType('resource');
      resources.forEach(resource => {
        if (this.shouldCollectResource(resource)) {
          this.metrics.resources[resource.name] = {
            duration: resource.duration,
            size: resource.transferSize,
            type: resource.initiatorType
          };
        }
      });
    }
  
    // Real User Monitoring
    setupUserMonitoring() {
      if (this.shouldMonitorUser()) {
        this.trackInteractions();
        this.measureFirstPaint();
        this.measureLargestContentfulPaint();
      }
    }
  
    trackInteractions() {
      const interactionMetrics = {
        clicks: new Set(),
        scrolls: 0,
        keypresses: 0
      };
  
      document.addEventListener('click', (e) => {
        if (this.config.privacyMode === 'strict') {
          interactionMetrics.clicks.add('interaction-recorded');
        } else {
          interactionMetrics.clicks.add({
            timestamp: Date.now(),
            elementType: e.target.tagName
          });
        }
      });
  
      document.addEventListener('scroll', this.throttle(() => {
        interactionMetrics.scrolls++;
      }, 1000));
  
      document.addEventListener('keypress', () => {
        interactionMetrics.keypresses++;
      });
    }
  
    measureFirstPaint() {
      const paint = performance.getEntriesByType('paint');
      const firstPaint = paint.find(entry => entry.name === 'first-paint');
      if (firstPaint) {
        this.metrics.performance.firstPaint = firstPaint.startTime;
      }
    }
  
    measureLargestContentfulPaint() {
      new PerformanceObserver((entryList) => {
        const entries = entryList.getEntries();
        const lastEntry = entries[entries.length - 1];
        this.metrics.performance.largestContentfulPaint = lastEntry.startTime;
      }).observe({ entryTypes: ['largest-contentful-paint'] });
    }
  
    // Security Compliance
    setupErrorTracking() {
      window.addEventListener('error', (event) => {
        const errorData = this.sanitizeErrorData({
          message: event.error.message,
          stack: event.error.stack,
          timestamp: Date.now()
        });
        this.metrics.errors.push(errorData);
      });
    }
  
    sanitizeErrorData(errorData) {
      // Remove sensitive information
      delete errorData.stack;
      return errorData;
    }
  
    // Privacy Controls
    shouldMonitorUser() {
      const userConsent = this.getUserConsent();
      const randomSample = Math.random() * 100 <= this.config.sampleRate;
      return userConsent && randomSample;
    }
  
    getUserConsent() {
      try {
        return localStorage.getItem('performance_monitoring_consent') === 'true';
      } catch {
        return false;
      }
    }
  
    setUserConsent(consent) {
      try {
        localStorage.setItem('performance_monitoring_consent', consent);
      } catch {
        console.warn('Could not save user consent');
      }
    }
  
    shouldCollectResource(resource) {
      // Filter out potentially sensitive resources
      const sensitivePatterns = [
        /auth/i,
        /login/i,
        /personal/i,
        /private/i
      ];
      
      return !sensitivePatterns.some(pattern => 
        pattern.test(resource.name)
      );
    }
  
    // Resource Usage Monitoring
    monitorResourceUsage() {
      if ('memory' in performance) {
        this.metrics.resources.memory = {
          usedJSHeapSize: performance.memory.usedJSHeapSize,
          totalJSHeapSize: performance.memory.totalJSHeapSize
        };
      }
  
      // CPU Usage estimation
      let lastTime = performance.now();
      let frameCount = 0;
  
      const checkFrame = () => {
        const currentTime = performance.now();
        frameCount++;
  
        if (currentTime - lastTime > 1000) {
          const fps = Math.round((frameCount * 1000) / (currentTime - lastTime));
          this.metrics.resources.fps = fps;
          frameCount = 0;
          lastTime = currentTime;
        }
  
        requestAnimationFrame(checkFrame);
      };
  
      requestAnimationFrame(checkFrame);
    }
  
    // Utility Functions
    throttle(func, limit) {
      let inThrottle;
      return function(...args) {
        if (!inThrottle) {
          func.apply(this, args);
          inThrottle = true;
          setTimeout(() => inThrottle = false, limit);
        }
      };
    }
  
    // Data Export
    getMetrics() {
      return {
        ...this.metrics,
        timestamp: Date.now(),
        version: '1.0.0'
      };
    }
  
    // Clear stored metrics
    clearMetrics() {
      this.metrics = {
        performance: {},
        resources: {},
        errors: []
      };
    }
  }
  
  // Usage Example
  const monitor = new PerformanceMonitor({
    sampleRate: 10, // Monitor 10% of users
    privacyMode: 'strict',
    retentionDays: 30
  });
  
  // Export metrics every 5 minutes
  setInterval(() => {
    const metrics = monitor.getMetrics();
    // Send metrics to your analytics server
    console.log('Collected metrics:', metrics);
  }, 300000);