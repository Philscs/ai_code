class Graph {
    constructor() {
      this.adjacencyList = {};
    }
  
    addVertex(vertex) {
      if (!this.adjacencyList[vertex]) {
        this.adjacencyList[vertex] = [];
      }
    }
  
    addEdge(source, destination) {
      if (this.adjacencyList[source] && this.adjacencyList[destination]) {
        this.adjacencyList[source].push(destination);
        this.adjacencyList[destination].push(source); // For undirected graph
      } else {
        throw new Error('Vertex not found');
      }
    }
  
    dfs(start) {
      const visited = new Set();
      const stack = [start];
      while (stack.length > 0) {
        const vertex = stack.pop();
        if (!visited.has(vertex)) {
          console.log(`Visited: ${vertex}`);
          visited.add(vertex);
          stack.push(...this.adjacencyList[vertex]);
        }
      }
    }
  
    bfs(start) {
      const visited = new Set();
      const queue = [start];
      while (queue.length > 0) {
        const vertex = queue.shift();
        if (!visited.has(vertex)) {
          console.log(`Visited: ${vertex}`);
          visited.add(vertex);
          queue.push(...this.adjacencyList[vertex]);
        }
      }
    }
  
    shortestPath(start, end) {
      const distance = {};
      const previous = {};
      const queue = [start];
      while (queue.length > 0) {
        const vertex = queue.shift();
        if (!distance[vertex]) {
          distance[vertex] = Infinity;
          previous[vertex] = null;
          for (const neighbor of this.adjacencyList[vertex]) {
            if (!distance[neighbor]) {
              distance[neighbor] = distance[vertex] + 1;
              previous[neighbor] = vertex;
              queue.push(neighbor);
            }
          }
        }
      }
      let path = [end];
      while (path[0] !== start) {
        path.unshift(previous[path[0]]);
      }
      return { distance: distance[end], path };
    }
  
    hasCycle(start, end) {
      const visited = new Set();
      const currentPath = [];
      function dfs(vertex) {
        if (visited.has(vertex)) {
          return true;
        } else {
          visited.add(vertex);
          currentPath.push(vertex);
          for (const neighbor of this.adjacencyList[vertex]) {
            if (currentPath.includes(neighbor)) {
              return true;
            }
            if (dfs.call(this, neighbor)) {
              return true;
            }
          }
          currentPath.pop();
        }
      }
      dfs(start);
      return false;
    }
  
    memoryEfficientStorage() {
      // Only store edges
      const edgeSet = new Set();
      for (const source in this.adjacencyList) {
        for (const destination of this.adjacencyList[source]) {
          edgeSet.add(`${source}->${destination}`);
        }
      }
      return edgeSet;
    }
  
    concurrentAccessSafety() {
      // Use a lock to prevent concurrent access
      const lock = new Mutex();
      function traverse(vertex, callback) {
        return new Promise(resolve => {
          lock.acquire('traverse', async () => {
            if (this.adjacencyList[vertex]) {
              await callback(vertex);
              for (const neighbor of this.adjacencyList[vertex]) {
                await traverse(neighbor, callback);
              }
            }
          });
          resolve();
        }).then(() => {
          // Release the lock after traversal is complete
          return;
        });
      }
    }
  
    static getMutex() {
      return new Mutex();
    }
  }
  
  class Mutex {
    constructor() {
      this._locked = false;
      this._queue = [];
    }
  
    async acquire(key, callback) {
      if (this._locked) {
        throw new Error(`Mutex is locked by another thread`);
      } else {
        this._locked = true;
        try {
          await callback();
        } finally {
          this._locked = false;
          if (this._queue.length > 0) {
            const nextCallback = this._queue.shift();
            this.acquire(key, nextCallback);
          }
        }
      }
    }
  }
  