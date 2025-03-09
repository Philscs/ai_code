// Effiziente Knoten-Implementierung mit WeakMap für Speicheroptimierung
class Node {
    constructor(value) {
        this.value = value;
        // WeakMap für automatische Speicherbereinigung nicht mehr referenzierter Knoten
        this._edges = new WeakMap();
    }
}

class Graph {
    constructor(isDirected = false) {
        this.nodes = new Map();
        this.isDirected = isDirected;
        // Mutex für nebenläufigen Zugriff
        this.mutex = new AsyncLock();
    }

    // Hilfsfunktion für Thread-sicheren Zugriff
    async withLock(callback) {
        return await this.mutex.acquire('graphLock', async () => {
            return await callback();
        });
    }

    async addNode(value) {
        return await this.withLock(async () => {
            if (!this.nodes.has(value)) {
                this.nodes.set(value, new Node(value));
            }
            return this.nodes.get(value);
        });
    }

    async addEdge(fromValue, toValue, weight = 1) {
        return await this.withLock(async () => {
            const fromNode = await this.addNode(fromValue);
            const toNode = await this.addNode(toValue);
            
            fromNode._edges.set(toNode, weight);
            if (!this.isDirected) {
                toNode._edges.set(fromNode, weight);
            }
        });
    }

    // Tiefensuche (DFS)
    async depthFirstSearch(startValue, visitCallback) {
        const visited = new Set();
        const startNode = this.nodes.get(startValue);
        
        if (!startNode) return;

        const dfsRecursive = async (node) => {
            visited.add(node);
            await visitCallback(node.value);

            for (const [neighbor] of node._edges) {
                if (!visited.has(neighbor)) {
                    await dfsRecursive(neighbor);
                }
            }
        };

        await this.withLock(async () => {
            await dfsRecursive(startNode);
        });
    }

    // Breitensuche (BFS)
    async breadthFirstSearch(startValue, visitCallback) {
        return await this.withLock(async () => {
            const visited = new Set();
            const queue = [];
            const startNode = this.nodes.get(startValue);
            
            if (!startNode) return;
            
            queue.push(startNode);
            visited.add(startNode);

            while (queue.length > 0) {
                const node = queue.shift();
                await visitCallback(node.value);

                for (const [neighbor] of node._edges) {
                    if (!visited.has(neighbor)) {
                        visited.add(neighbor);
                        queue.push(neighbor);
                    }
                }
            }
        });
    }

    // Dijkstra's Algorithmus für kürzeste Pfade
    async shortestPath(startValue, endValue) {
        return await this.withLock(async () => {
            const distances = new Map();
            const previous = new Map();
            const unvisited = new Set();

            // Initialisierung
            for (const [value, node] of this.nodes) {
                distances.set(node, value === startValue ? 0 : Infinity);
                unvisited.add(node);
            }

            const startNode = this.nodes.get(startValue);
            if (!startNode) return null;

            while (unvisited.size > 0) {
                // Finde Knoten mit minimaler Distanz
                let current = Array.from(unvisited)
                    .reduce((min, node) => 
                        distances.get(node) < distances.get(min) ? node : min
                    );

                if (current.value === endValue) {
                    // Pfad rekonstruieren
                    const path = [];
                    while (current) {
                        path.unshift(current.value);
                        current = previous.get(current);
                    }
                    return path;
                }

                unvisited.delete(current);

                for (const [neighbor, weight] of current._edges) {
                    if (!unvisited.has(neighbor)) continue;

                    const alt = distances.get(current) + weight;
                    if (alt < distances.get(neighbor)) {
                        distances.set(neighbor, alt);
                        previous.set(neighbor, current);
                    }
                }
            }

            return null; // Kein Pfad gefunden
        });
    }

    // Zyklenerkennung mittels DFS
    async hasCycle() {
        return await this.withLock(async () => {
            const visited = new Set();
            const recursionStack = new Set();

            const hasCycleUtil = async (node) => {
                visited.add(node);
                recursionStack.add(node);

                for (const [neighbor] of node._edges) {
                    if (!visited.has(neighbor)) {
                        if (await hasCycleUtil(neighbor)) {
                            return true;
                        }
                    } else if (recursionStack.has(neighbor)) {
                        return true;
                    }
                }

                recursionStack.delete(node);
                return false;
            };

            for (const [_, node] of this.nodes) {
                if (!visited.has(node)) {
                    if (await hasCycleUtil(node)) {
                        return true;
                    }
                }
            }

            return false;
        });
    }
}

// Hilfklasse für Mutex-Implementation
class AsyncLock {
    constructor() {
        this.locks = new Map();
    }

    async acquire(key, callback) {
        if (!this.locks.has(key)) {
            this.locks.set(key, Promise.resolve());
        }

        const currentLock = this.locks.get(key);
        
        let release;
        const newLock = new Promise(resolve => {
            release = resolve;
        });

        this.locks.set(key, newLock);

        await currentLock;

        try {
            const result = await callback();
            return result;
        } finally {
            release();
        }
    }
}

// Beispielnutzung:
async function example() {
    const graph = new Graph();
    
    // Knoten und Kanten hinzufügen
    await graph.addEdge('A', 'B', 4);
    await graph.addEdge('A', 'C', 2);
    await graph.addEdge('B', 'E', 3);
    await graph.addEdge('C', 'D', 2);
    await graph.addEdge('D', 'E', 3);
    
    // DFS durchführen
    console.log('DFS Traversierung:');
    await graph.depthFirstSearch('A', value => console.log(value));
    
    // Kürzesten Pfad finden
    const path = await graph.shortestPath('A', 'E');
    console.log('Kürzester Pfad von A nach E:', path);
    
    // Zyklen erkennen
    const hasCycle = await graph.hasCycle();
    console.log('Graph enthält Zyklen:', hasCycle);
}