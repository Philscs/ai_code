class MemoryLeakDetector {
    constructor() {
        this.eventListeners = new Map();
        this.domNodes = new WeakMap();
        this.memorySnapshots = [];
    }

    // Track added event listeners
    trackEventListener(target, type, listener) {
        if (!this.eventListeners.has(target)) {
            this.eventListeners.set(target, []);
        }
        this.eventListeners.get(target).push({ type, listener });
    }

    // Remove event listener and update tracking
    removeEventListener(target, type, listener) {
        if (this.eventListeners.has(target)) {
            const listeners = this.eventListeners.get(target).filter(
                (l) => l.type !== type || l.listener !== listener
            );
            this.eventListeners.set(target, listeners);
        }
        target.removeEventListener(type, listener);
    }

    // Detect Event-Listener Leaks
    detectEventListenerLeaks() {
        for (const [target, listeners] of this.eventListeners.entries()) {
            if (listeners.length > 0) {
                console.warn("Potential Event Listener Leak:", target, listeners);
            }
        }
    }

    // Track DOM Nodes
    trackDOMNode(node) {
        this.domNodes.set(node, true);
    }

    // Detect DOM Node Leaks
    detectDOMNodeLeaks() {
        for (const node of this.domNodes.keys()) {
            if (!document.contains(node)) {
                console.warn("Detached DOM Node Detected:", node);
            }
        }
    }

    // Take a memory snapshot
    takeMemorySnapshot() {
        if (window.performance && performance.memory) {
            this.memorySnapshots.push({
                time: Date.now(),
                memory: { ...performance.memory },
            });
            console.log("Memory Snapshot Taken:", this.memorySnapshots);
        } else {
            console.warn("Memory profiling is not supported in this browser.");
        }
    }

    // Compare memory snapshots
    compareMemorySnapshots() {
        if (this.memorySnapshots.length < 2) {
            console.warn("Not enough memory snapshots to compare.");
            return;
        }

        const [prev, current] = this.memorySnapshots.slice(-2);
        console.log("Memory Snapshot Comparison:", {
            previous: prev.memory,
            current: current.memory,
            difference: {
                jsHeapSizeUsed:
                    current.memory.jsHeapSizeUsed - prev.memory.jsHeapSizeUsed,
                totalJSHeapSize:
                    current.memory.totalJSHeapSize - prev.memory.totalJSHeapSize,
            },
        });
    }

    // Detect circular references using WeakMap
    detectCircularReferences(obj) {
        const visited = new WeakSet();

        const detect = (value) => {
            if (value && typeof value === "object") {
                if (visited.has(value)) {
                    console.warn("Circular Reference Detected:", value);
                    return true;
                }
                visited.add(value);
                for (const key in value) {
                    detect(value[key]);
                }
            }
            return false;
        };

        return detect(obj);
    }

    // Comprehensive analysis for memory leaks
    analyze() {
        console.log("Starting Memory Leak Analysis...");
        this.detectEventListenerLeaks();
        this.detectDOMNodeLeaks();
    }
}

// Example Usage
const detector = new MemoryLeakDetector();

// Track an event listener
const button = document.createElement('button');
button.textContent = 'Click Me';
document.body.appendChild(button);

detector.trackEventListener(button, 'click', () => console.log('Button clicked!'));
button.addEventListener('click', () => console.log('Button clicked!'));

detector.trackDOMNode(button);

detector.takeMemorySnapshot(); // Initial memory snapshot

// Simulate DOM removal
setTimeout(() => {
    document.body.removeChild(button);
    detector.analyze(); // Detect leaks
    detector.takeMemorySnapshot(); // Take another snapshot
    detector.compareMemorySnapshots(); // Compare snapshots
}, 2000);
