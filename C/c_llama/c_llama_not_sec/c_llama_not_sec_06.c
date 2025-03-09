#include <stdio.h>
#include <stdlib.h>

#define MAX_NODES 100

typedef struct {
    int adjacentNodes[2]; // 1 für nächste Knoten, -1 für vorhergehenden Knoten
    char visited;
} Graph;

Graph graph[MAX_NODES];

// Funktion zum Erstellen eines neuen Graphs
void createGraph(Graph *graph) {
    for (int i = 0; i < MAX_NODES; i++) {
        graph[i].adjacentNodes[0] = -1;
        graph[i].adjacentNodes[1] = -1;
        graph[i].visited = 0;
    }
}

// Funktion zum Hinzufügen einer Verbindung zwischen zwei Knoten
void addEdge(int from, int to) {
    if (graph[from].adjacentNodes[0] == -1 || graph[to].adjacentNodes[1] == -1) {
        graph[from].adjacentNodes[0] = to;
        graph[to].adjacentNodes[1] = from;
    }
}

// Funktion zum Durchführen von Dijkstra
int dijkstra(int startNode, int endNode) {
    int distance[MAX_NODES];
    for (int i = 0; i < MAX_NODES; i++) {
        distance[i] = -1;
    }
    distance[startNode] = 0;

    int queue[MAX_NODES];
    int front = 0;
    int rear = 1;

    while (front < rear) {
        int currentNode = queue[front++];
        graph[currentNode].visited = 1;

        if (currentNode == endNode) break;

        for (int i = 0; i < 2; i++) {
            int adjacentNode = graph[currentNode].adjacentNodes[i];
            if (graph[adjacentNode].visited && distance[adjacentNode] >= 0) continue;

            if (distance[adjacentNode] == -1 || distance[currentNode] + 1 < 
distance[adjacentNode]) {
                distance[adjacentNode] = distance[currentNode] + 1;
                graph[adjacentNode].visited = 1;
                queue[rear++] = adjacentNode;
            }
        }
    }

    return distance[endNode];
}

// Funktion zum Durchführen von A*
int aStar(int startNode, int endNode) {
    int distance[MAX_NODES];
    for (int i = 0; i < MAX_NODES; i++) {
        distance[i] = -1;
    }
    distance[startNode] = 0;

    int queue[MAX_NODES];
    int front = 0;
    int rear = 1;

    while (front < rear) {
        int currentNode = queue[front++];
        graph[currentNode].visited = 1;

        if (currentNode == endNode) break;

        for (int i = 0; i < 2; i++) {
            int adjacentNode = graph[currentNode].adjacentNodes[i];
            if (graph[adjacentNode].visited && distance[adjacentNode] >= 0) continue;

            if (distance[adjacentNode] == -1 || distance[currentNode] + 1 < 
distance[adjacentNode]) {
                distance[adjacentNode] = distance[currentNode] + 1;
                graph[adjacentNode].visited = 1;
                float heuristicValue = calculateHeuristic(adjacentNode, endNode);
                if (heuristicValue > 0) {
                    float totalCost = distance[adjacentNode] + heuristicValue;
                    int betterCandidate = findBetterCandidate(queue, rear, adjacentNode, 
totalCost);
                    if (betterCandidate != -1) {
                        graph[betterCandidate].visited = 1;
                        queue[front++] = betterCandidate;
                    }
                } else {
                    queue[rear++] = adjacentNode;
                }
            }
        }
    }

    return distance[endNode];
}

// Funktion zum Berechnen des Heuristischen Wertes
float calculateHeuristic(int currentNode, int endNode) {
    // Hier müssen Sie den Heuristischen Wert implementieren.
    // Ein einfacher Ansatz ist die Benutzung eines Manhattan-Distanz-Algorithmus.
    return abs(graph[currentNode].adjacentNodes[0] - graph[endNode].adjacentNodes[1]);
}

// Funktion zum Finden des besseren Kandidaten
int findBetterCandidate(int queue[], int rear, int currentNode, float totalCost) {
    for (int i = 0; i < rear; i++) {
        if (queue[i] == -1 || totalCost <= distance[queue[i]]) continue;

        return queue[i];
    }
    return -1;
}

// Hauptschleife
int main() {
    Graph graph;
    createGraph(&graph);

    // Beispiele für Verbindungen zwischen Knoten hinzufügen
    addEdge(0, 1);
    addEdge(0, 2);
    addEdge(1, 3);
    addEdge(1, 4);
    addEdge(2, 5);
    addEdge(3, 6);

    // Durchführen von Dijkstra und A*
    int startNode = 0;
    int endNode = 6;

    float dijkstraResult = dijkstra(startNode, endNode);
    float aStarResult = aStar(startNode, endNode);

    printf("Dijkstra-Ergebnis: %f\n", dijkstraResult);
    printf("A* Ergebnis: %f\n", aStarResult);

    return 0;
}