class Graph {
    constructor() {
      this.adjacencyList = new Map(); // Speichereffiziente Speicherung
    }
  
    // Knoten hinzufügen
    addVertex(vertex) {
      if (!this.adjacencyList.has(vertex)) {
        this.adjacencyList.set(vertex, new Set());
      }
    }
  
    // Kante hinzufügen
    addEdge(vertex1, vertex2) {
      this.addVertex(vertex1);
      this.addVertex(vertex2);
      this.adjacencyList.get(vertex1).add(vertex2);
      this.adjacencyList.get(vertex2).add(vertex1); // Für ungerichteten Graph
    }
  
    // DFS (rekursiv)
    dfsRecursive(start, visited = new Set()) {
      visited.add(start);
      console.log(start);
      for (const neighbor of this.adjacencyList.get(start)) {
        if (!visited.has(neighbor)) {
          this.dfsRecursive(neighbor, visited);
        }
      }
    }
  
    // BFS (iterativ)
    bfs(start) {
      const visited = new Set();
      const queue = [start];
      visited.add(start);
  
      while (queue.length) {
        const vertex = queue.shift();
        console.log(vertex);
  
        for (const neighbor of this.adjacencyList.get(vertex)) {
          if (!visited.has(neighbor)) {
            visited.add(neighbor);
            queue.push(neighbor);
          }
        }
      }
    }
  
    // Kürzester Pfad (Breitensuche)
    shortestPath(start, end) {
      const queue = [[start]];
      const visited = new Set([start]);
  
      while (queue.length) {
        const path = queue.shift();
        const node = path[path.length - 1];
  
        if (node === end) return path;
  
        for (const neighbor of this.adjacencyList.get(node)) {
          if (!visited.has(neighbor)) {
            visited.add(neighbor);
            queue.push([...path, neighbor]);
          }
        }
      }
  
      return null; // Kein Pfad gefunden
    }
  
    // Zyklenerkennung (DFS)
    detectCycle() {
      const visited = new Set();
      const stack = new Set();
  
      const dfs = (vertex, parent) => {
        visited.add(vertex);
        stack.add(vertex);
  
        for (const neighbor of this.adjacencyList.get(vertex)) {
          if (!visited.has(neighbor)) {
            if (dfs(neighbor, vertex)) return true;
          } else if (neighbor !== parent && stack.has(neighbor)) {
            return true;
          }
        }
  
        stack.delete(vertex);
        return false;
      };
  
      for (const vertex of this.adjacencyList.keys()) {
        if (!visited.has(vertex)) {
          if (dfs(vertex, null)) return true;
        }
      }
  
      return false;
    }
  
    // Gleichzeitige Zugriffssicherheit (Mutex-Simulation)
    async withLock(action) {
      if (this.lock) {
        await this.lock;
      }
  
      let resolve;
      this.lock = new Promise((res) => (resolve = res));
      try {
        await action();
      } finally {
        resolve();
        this.lock = null;
      }
    }
  }
  
  // Beispielnutzung
  const graph = new Graph();
  graph.addEdge('A', 'B');
  graph.addEdge('A', 'C');
  graph.addEdge('B', 'D');
  graph.addEdge('C', 'D');
  
  graph.withLock(async () => {
    console.log('DFS:');
    graph.dfsRecursive('A');
  
    console.log('BFS:');
    graph.bfs('A');
  
    console.log('Shortest Path from A to D:');
    console.log(graph.shortestPath('A', 'D'));
  
    console.log('Contains Cycle:', graph.detectCycle());
  });
  