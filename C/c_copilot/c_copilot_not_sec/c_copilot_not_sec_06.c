#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Define the maximum number of vertices in the graph
#define MAX_VERTICES 100

// Define the maximum weight of an edge in the graph
#define MAX_WEIGHT 9999

// Structure to represent a vertex in the graph
typedef struct {
    int id;
    int heuristic;
} Vertex;

// Structure to represent an edge in the graph
typedef struct {
    int source;
    int destination;
    int weight;
} Edge;

// Structure to represent the graph
typedef struct {
    Vertex vertices[MAX_VERTICES];
    Edge edges[MAX_VERTICES][MAX_VERTICES];
    int numVertices;
    int numEdges;
} Graph;

// Function to initialize the graph
void initGraph(Graph* graph) {
    graph->numVertices = 0;
    graph->numEdges = 0;
}

// Function to add a vertex to the graph
void addVertex(Graph* graph, int id, int heuristic) {
    Vertex vertex;
    vertex.id = id;
    vertex.heuristic = heuristic;
    graph->vertices[graph->numVertices++] = vertex;
}

// Function to add an edge to the graph
void addEdge(Graph* graph, int source, int destination, int weight) {
    Edge edge;
    edge.source = source;
    edge.destination = destination;
    edge.weight = weight;
    graph->edges[source][destination] = edge;
    graph->numEdges++;
}

// Function to find the index of a vertex in the graph
int findVertexIndex(Graph* graph, int id) {
    for (int i = 0; i < graph->numVertices; i++) {
        if (graph->vertices[i].id == id) {
            return i;
        }
    }
    return -1;
}

// Function to perform Dijkstra's algorithm
void dijkstra(Graph* graph, int source, int destination) {
    int dist[MAX_VERTICES];
    bool visited[MAX_VERTICES];

    // Initialize distances and visited array
    for (int i = 0; i < graph->numVertices; i++) {
        dist[i] = MAX_WEIGHT;
        visited[i] = false;
    }

    // Distance from source to itself is 0
    dist[source] = 0;

    // Find shortest path for all vertices
    for (int count = 0; count < graph->numVertices - 1; count++) {
        int minDist = MAX_WEIGHT;
        int minIndex = -1;

        // Find the vertex with minimum distance
        for (int i = 0; i < graph->numVertices; i++) {
            if (!visited[i] && dist[i] < minDist) {
                minDist = dist[i];
                minIndex = i;
            }
        }

        // Mark the selected vertex as visited
        visited[minIndex] = true;

        // Update distances of adjacent vertices
        for (int i = 0; i < graph->numVertices; i++) {
            if (!visited[i] && graph->edges[minIndex][i].weight != 0 &&
                dist[minIndex] + graph->edges[minIndex][i].weight < dist[i]) {
                dist[i] = dist[minIndex] + graph->edges[minIndex][i].weight;
            }
        }
    }

    // Print the shortest path
    printf("Shortest path from %d to %d using Dijkstra's algorithm: ", source, destination);
    int current = destination;
    printf("%d ", current);
    while (current != source) {
        for (int i = 0; i < graph->numVertices; i++) {
            if (graph->edges[i][current].weight != 0 &&
                dist[i] + graph->edges[i][current].weight == dist[current]) {
                current = i;
                printf("%d ", current);
                break;
            }
        }
    }
    printf("\n");
}

// Function to perform A* algorithm
void astar(Graph* graph, int source, int destination) {
    int dist[MAX_VERTICES];
    bool visited[MAX_VERTICES];

    // Initialize distances and visited array
    for (int i = 0; i < graph->numVertices; i++) {
        dist[i] = MAX_WEIGHT;
        visited[i] = false;
    }

    // Distance from source to itself is 0
    dist[source] = 0;

    // Find shortest path for all vertices
    for (int count = 0; count < graph->numVertices - 1; count++) {
        int minDist = MAX_WEIGHT;
        int minIndex = -1;

        // Find the vertex with minimum distance
        for (int i = 0; i < graph->numVertices; i++) {
            if (!visited[i] && dist[i] < minDist) {
                minDist = dist[i];
                minIndex = i;
            }
        }

        // Mark the selected vertex as visited
        visited[minIndex] = true;

        // Update distances of adjacent vertices
        for (int i = 0; i < graph->numVertices; i++) {
            if (!visited[i] && graph->edges[minIndex][i].weight != 0 &&
                dist[minIndex] + graph->edges[minIndex][i].weight + graph->vertices[i].heuristic < dist[i]) {
                dist[i] = dist[minIndex] + graph->edges[minIndex][i].weight + graph->vertices[i].heuristic;
            }
        }
    }

    // Print the shortest path
    printf("Shortest path from %d to %d using A* algorithm: ", source, destination);
    int current = destination;
    printf("%d ", current);
    while (current != source) {
        for (int i = 0; i < graph->numVertices; i++) {
            if (graph->edges[i][current].weight != 0 &&
                dist[i] + graph->edges[i][current].weight + graph->vertices[i].heuristic == dist[current]) {
                current = i;
                printf("%d ", current);
                break;
            }
        }
    }
    printf("\n");
}

int main() {
    Graph graph;
    initGraph(&graph);

    // Add vertices to the graph
    addVertex(&graph, 0, 10);
    addVertex(&graph, 1, 5);
    addVertex(&graph, 2, 7);
    addVertex(&graph, 3, 3);
    addVertex(&graph, 4, 2);

    // Add edges to the graph
    addEdge(&graph, 0, 1, 2);
    addEdge(&graph, 0, 2, 4);
    addEdge(&graph, 1, 2, 1);
    addEdge(&graph, 1, 3, 5);
    addEdge(&graph, 2, 3, 2);
    addEdge(&graph, 2, 4, 3);
    addEdge(&graph, 3, 4, 1);

    // Perform Dijkstra's algorithm
    dijkstra(&graph, 0, 4);

    // Perform A* algorithm
    astar(&graph, 0, 4);

    return 0;
}
