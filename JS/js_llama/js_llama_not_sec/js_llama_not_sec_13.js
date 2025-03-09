class MemoryLeakDetector {
    constructor() {
      this.leaks = [];
      this.memorySnapshots = {};
    }
  
    addListener(node, listener) {
      const eventHandler = (event) => {
        listener(event);
      };
      node.addEventListener('click', eventHandler);
      return () => {
        node.removeEventListener('click', eventHandler);
      };
    }
  
    detectLeaks() {
      // Event-Listener-Leaks
      this.leaks.push(...this.getEventListeners());
  
      // Zirkuläre Referenzen
      const circularReferences = this.getCircularReferences();
      if (circularReferences.length > 0) {
        console.log('Zirkuläre Referenzen gefunden:', circularReferences);
      }
  
      // DOM-Node Leaks
      const domLeaks = this.getDomLeaks();
      if (domLeaks.length > 0) {
        console.log('DOM-Node Leaks identifiziert:', domLeaks);
      }
  
      return this.leaks;
    }
  
    getEventListeners() {
      const eventListeners = [];
      const eventHandlers = Object.values(this.memorySnapshots).map((snapshot) => {
        const handlers = Object.values(snapshot);
        handlers.forEach((handler) => {
          if (typeof handler === 'function') {
            eventListeners.push(handler);
          }
        });
        return handlers;
      });
      return [...new Set(eventListeners)];
    }
  
    getCircularReferences() {
      const visited = new Set();
      const circularReferences = [];
  
      function getCircularReferences(node) {
        if (visited.has(node)) {
          circularReferences.push(node);
        } else {
          visited.add(node);
        }
        for (const property in node) {
          if (typeof node[property] === 'object') {
            getCircularReferences(node[property]);
          }
        }
      }
  
      const domNodes = document.querySelectorAll('*');
      domNodes.forEach((node) => {
        getCircularReferences(node);
      });
  
      return circularReferences;
    }
  
    getDomLeaks() {
      const domLeaks = [];
      for (const node in this.memorySnapshots) {
        if (this.memorySnapshots[node].length > 0 && !node.contains(document.body)) {
          domLeaks.push(node);
        }
      }
      return [...new Set(domLeaks)];
    }
  
    compareMemorySnapshots() {
      const differences = [];
      for (const node in this.memorySnapshots) {
        if (!Object.values(this.memorySnapshots).includes(node)) {
          differences.push([node, true]);
        } else {
          const snapshot = 
  Object.values(this.memorySnapshots)[Object.keys(this.memorySnapshots).indexOf(node)];
          for (const element of snapshot) {
            if (!this.memorySnapshots[node].includes(element)) {
              differences.push([node, false]);
              break;
            }
          }
        }
      }
      return differences;
    }
  
    compareLeaks() {
      const newLeaks = this.detectLeaks();
      const oldLeaks = Object.values(this.leaks);
      const differences = [];
      for (const leak of newLeaks) {
        if (!oldLeaks.includes(leak)) {
          differences.push([leak, true]);
        } else {
          const index = oldLeaks.indexOf(leak);
          if (oldLeaks[index] !== leak) {
            differences.push([leak, false]);
          }
        }
      }
      return differences;
    }
  
    takeMemorySnapshot() {
      this.memorySnapshots = {};
      for (const node in document.querySelectorAll('*')) {
        this.memorySnapshots[node] = [];
      }
    }
  }
  
  // Beispiel-Herausforderung
  const detector = new MemoryLeakDetector();
  
  // Erstes Memory-Snapshot
  detector.takeMemorySnapshot();
  const domNode = document.createElement('div');
  document.body.appendChild(domNode);
  detector.addListener(domNode, (event) => {
    console.log(`Event ausgelöst: ${event.type}`);
  });
  
  // Zweitens Memory-Snapshot
  detector.takeMemorySnapshot();
  
  // Überprüfen der Leaks
  console.log('Leaks identifiziert:', detector.compareLeaks());
  
  // Vergleichen von Memory-Snapshots
  const differences = detector.compareMemorySnapshots();
  console.log('Differenzen zwischen Memorie-Snapshotten:', differences);
  