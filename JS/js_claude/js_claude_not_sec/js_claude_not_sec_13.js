class MemoryLeakDetector {
    constructor() {
      this.eventListenerRegistry = new Map();
      this.previousSnapshot = null;
      this.nodeRegistry = new WeakMap();
      this.detachedNodes = new Set();
    }
  
    // Event Listener Tracking
    trackEventListener(element, eventType, handler) {
      if (!this.eventListenerRegistry.has(element)) {
        this.eventListenerRegistry.set(element, new Map());
      }
      
      const elementListeners = this.eventListenerRegistry.get(element);
      if (!elementListeners.has(eventType)) {
        elementListeners.set(eventType, new Set());
      }
      
      elementListeners.get(eventType).add(handler);
    }
  
    detectEventListenerLeaks() {
      const leaks = [];
      
      this.eventListenerRegistry.forEach((listeners, element) => {
        if (!document.contains(element)) {
          listeners.forEach((handlers, eventType) => {
            leaks.push({
              element: element,
              eventType: eventType,
              handlersCount: handlers.size
            });
          });
        }
      });
  
      return leaks;
    }
  
    // Circular Reference Detection
    detectCircularReferences(obj, seen = new WeakSet()) {
      const circularRefs = [];
  
      const detect = (current, path = []) => {
        if (current && typeof current === 'object') {
          if (seen.has(current)) {
            circularRefs.push([...path]);
            return;
          }
          
          seen.add(current);
          
          Object.entries(current).forEach(([key, value]) => {
            detect(value, [...path, key]);
          });
        }
      };
  
      detect(obj);
      return circularRefs;
    }
  
    // DOM Node Leak Detection
    trackNode(node) {
      this.nodeRegistry.set(node, {
        addedAt: Date.now(),
        isAttached: document.contains(node)
      });
    }
  
    detectDetachedNodes() {
      const now = Date.now();
      const threshold = 30000; // 30 seconds threshold
  
      this.nodeRegistry.forEach((info, node) => {
        const isCurrentlyAttached = document.contains(node);
        
        if (!isCurrentlyAttached && (now - info.addedAt > threshold)) {
          this.detachedNodes.add(node);
        } else if (isCurrentlyAttached) {
          this.detachedNodes.delete(node);
        }
      });
  
      return Array.from(this.detachedNodes);
    }
  
    // Memory Snapshot Comparison
    takeMemorySnapshot() {
      return new Promise((resolve) => {
        if (window.performance && performance.memory) {
          const snapshot = {
            timestamp: Date.now(),
            usedJSHeapSize: performance.memory.usedJSHeapSize,
            totalJSHeapSize: performance.memory.totalJSHeapSize,
            detachedNodes: this.detectDetachedNodes().length,
            eventListenerLeaks: this.detectEventListenerLeaks().length
          };
  
          resolve(snapshot);
        } else {
          resolve(null);
        }
      });
    }
  
    async compareSnapshots() {
      const currentSnapshot = await this.takeMemorySnapshot();
      
      if (!currentSnapshot || !this.previousSnapshot) {
        this.previousSnapshot = currentSnapshot;
        return null;
      }
  
      const comparison = {
        timespan: currentSnapshot.timestamp - this.previousSnapshot.timestamp,
        heapDiff: currentSnapshot.usedJSHeapSize - this.previousSnapshot.usedJSHeapSize,
        detachedNodesDiff: currentSnapshot.detachedNodes - this.previousSnapshot.detachedNodes,
        eventLeaksDiff: currentSnapshot.eventListenerLeaks - this.previousSnapshot.eventListenerLeaks,
        growthRate: (currentSnapshot.usedJSHeapSize - this.previousSnapshot.usedJSHeapSize) / 
                   (currentSnapshot.timestamp - this.previousSnapshot.timestamp)
      };
  
      this.previousSnapshot = currentSnapshot;
      return comparison;
    }
  
    // Utility method to start monitoring
    startMonitoring(intervalMs = 10000) {
      // Initial snapshot
      this.takeMemorySnapshot();
  
      // Set up periodic monitoring
      return setInterval(async () => {
        const comparison = await this.compareSnapshots();
        
        if (comparison) {
          // Check for significant memory growth
          if (comparison.growthRate > 1000000) { // 1MB per second threshold
            console.warn('Significant memory growth detected:', comparison);
          }
  
          // Check for accumulated detached nodes
          if (comparison.detachedNodesDiff > 10) {
            console.warn('Significant increase in detached nodes:', comparison);
          }
  
          // Check for event listener leaks
          if (comparison.eventLeaksDiff > 0) {
            console.warn('New event listener leaks detected:', comparison);
          }
        }
      }, intervalMs);
    }
  }
  
  // Usage example:
  const detector = new MemoryLeakDetector();
  
  // Track event listeners
  const originalAddEventListener = Element.prototype.addEventListener;
  Element.prototype.addEventListener = function(type, handler, options) {
    detector.trackEventListener(this, type, handler);
    return originalAddEventListener.call(this, type, handler, options);
  };
  
  // Track DOM nodes
  const observer = new MutationObserver((mutations) => {
    mutations.forEach((mutation) => {
      mutation.addedNodes.forEach((node) => {
        detector.trackNode(node);
      });
    });
  });
  
  observer.observe(document.body, {
    childList: true,
    subtree: true
  });
  
  // Start monitoring
  const monitoringInterval = detector.startMonitoring(5000); // Check every 5 seconds
  
  // Example of checking for circular references
  const checkCircularRefs = (obj) => {
    const circles = detector.detectCircularReferences(obj);
    if (circles.length > 0) {
      console.warn('Circular references detected:', circles);
    }
  };