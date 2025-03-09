class Graph {
  constructor() {
    this.vertices = new Map();
  }

  addVertex(vertex) {
    if (!this.vertices.has(vertex)) {
      this.vertices.set(vertex, new Set());
    }
  }

  addEdge(vertex1, vertex2) {
    if (this.vertices.has(vertex1) && this.vertices.has(vertex2)) {
      this.vertices.get(vertex1).add(vertex2);
      this.vertices.get(vertex2).add(vertex1);
    }
  }

  removeEdge(vertex1, vertex2) {
    if (this.vertices.has(vertex1) && this.vertices.has(vertex2)) {
      this.vertices.get(vertex1).delete(vertex2);
      this.vertices.get(vertex2).delete(vertex1);
    }
  }

  removeVertex(vertex) {
    if (this.vertices.has(vertex)) {
      const adjacentVertices = this.vertices.get(vertex);
      for (const adjacentVertex of adjacentVertices) {
        this.vertices.get(adjacentVertex).delete(vertex);
      }
      this.vertices.delete(vertex);
    }
  }

  dfs(startVertex) {
    const visited = new Set();
    const stack = [startVertex];
    const traversalOrder = [];

    while (stack.length > 0) {
      const currentVertex = stack.pop();
      if (!visited.has(currentVertex)) {
        visited.add(currentVertex);
        traversalOrder.push(currentVertex);

        const adjacentVertices = this.vertices.get(currentVertex);
        for (const adjacentVertex of adjacentVertices) {
          stack.push(adjacentVertex);
        }
      }
    }

    return traversalOrder;
  }

  bfs(startVertex) {
    const visited = new Set();
    const queue = [startVertex];
    const traversalOrder = [];

    while (queue.length > 0) {
      const currentVertex = queue.shift();
      if (!visited.has(currentVertex)) {
        visited.add(currentVertex);
        traversalOrder.push(currentVertex);

        const adjacentVertices = this.vertices.get(currentVertex);
        for (const adjacentVertex of adjacentVertices) {
          queue.push(adjacentVertex);
        }
      }
    }

    return traversalOrder;
  }

  shortestPath(startVertex, endVertex) {
    const visited = new Set();
    const queue = [[startVertex]];
    
    if (startVertex === endVertex) {
      return [startVertex];
    }

    while (queue.length > 0) {
      const path = queue.shift();
      const currentVertex = path[path.length - 1];

      if (!visited.has(currentVertex)) {
        visited.add(currentVertex);

        const adjacentVertices = this.vertices.get(currentVertex);
        for (const adjacentVertex of adjacentVertices) {
          const newPath = [...path, adjacentVertex];
          if (adjacentVertex === endVertex) {
            return newPath;
          }
          queue.push(newPath);
        }
      }
    }

    return [];
  }

  hasCycle() {
    const visited = new Set();
    for (const vertex of this.vertices.keys()) {
      if (!visited.has(vertex) && this.hasCycleUtil(vertex, visited, null)) {
        return true;
      }
    }
    return false;
  }

  hasCycleUtil(vertex, visited, parent) {
    visited.add(vertex);

    const adjacentVertices = this.vertices.get(vertex);
    for (const adjacentVertex of adjacentVertices) {
      if (!visited.has(adjacentVertex)) {
        if (this.hasCycleUtil(adjacentVertex, visited, vertex)) {
          return true;
        }
      } else if (adjacentVertex !== parent) {
        return true;
      }
    }

    return false;
  }
}

class MemoryEfficientGraph {
  constructor() {
    this.vertices = new Map();
  }

  addVertex(vertex) {
    if (!this.vertices.has(vertex)) {
      this.vertices.set(vertex, new Map());
    }
  }

  addEdge(vertex1, vertex2, weight) {
    if (this.vertices.has(vertex1) && this.vertices.has(vertex2)) {
      this.vertices.get(vertex1).set(vertex2, weight);
      this.vertices.get(vertex2).set(vertex1, weight);
    }
  }

  removeEdge(vertex1, vertex2) {
    if (this.vertices.has(vertex1) && this.vertices.has(vertex2)) {
      this.vertices.get(vertex1).delete(vertex2);
      this.vertices.get(vertex2).delete(vertex1);
    }
  }

  removeVertex(vertex) {
    if (this.vertices.has(vertex)) {
      const adjacentVertices = this.vertices.get(vertex);
      for (const adjacentVertex of adjacentVertices.keys()) {
        this.vertices.get(adjacentVertex).delete(vertex);
      }
      this.vertices.delete(vertex);
    }
  }
}

class ConcurrentAccessSafeGraph {
  constructor() {
    this.vertices = new Map();
    this.locks = new Map();
  }

  addVertex(vertex) {
    if (!this.vertices.has(vertex)) {
      this.vertices.set(vertex, new Set());
      this.locks.set(vertex, new Lock());
    }
  }

  addEdge(vertex1, vertex2) {
    if (this.vertices.has(vertex1) && this.vertices.has(vertex2)) {
      this.locks.get(vertex1).acquire();
      this.locks.get(vertex2).acquire();
      this.vertices.get(vertex1).add(vertex2);
      this.vertices.get(vertex2).add(vertex1);
      this.locks.get(vertex1).release();
      this.locks.get(vertex2).release();
    }
  }

  removeEdge(vertex1, vertex2) {
    if (this.vertices.has(vertex1) && this.vertices.has(vertex2)) {
      this.locks.get(vertex1).acquire();
      this.locks.get(vertex2).acquire();
      this.vertices.get(vertex1).delete(vertex2);
      this.vertices.get(vertex2).delete(vertex1);
      this.locks.get(vertex1).release();
      this.locks.get(vertex2).release();
    }
  }

  removeVertex(vertex) {
    if (this.vertices.has(vertex)) {
      this.locks.get(vertex).acquire();
      const adjacentVertices = this.vertices.get(vertex);
      for (const adjacentVertex of adjacentVertices) {
        this.locks.get(adjacentVertex).acquire();
        this.vertices.get(adjacentVertex).delete(vertex);
        this.locks.get(adjacentVertex).release();
      }
      this.vertices.delete(vertex);
      this.locks.delete(vertex);
    }
  }
}

class Lock {
  constructor() {
    this.isLocked = false;
    this.waitQueue = [];
  }

  acquire() {
    if (this.isLocked) {
      return new Promise((resolve) => {
        this.waitQueue.push(resolve);
      });
    } else {
      this.isLocked = true;
      return Promise.resolve();
    }
  }

  release() {
    if (this.waitQueue.length > 0) {
      const nextResolve = this.waitQueue.shift();
      nextResolve();
    } else {
      this.isLocked = false;
    }
  }
}
