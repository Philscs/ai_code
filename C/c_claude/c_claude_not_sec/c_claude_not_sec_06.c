#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <time.h>

#define MAX_NODES 1000
#define INF DBL_MAX

// Struktur für einen Knoten im Netzwerk
typedef struct {
    int id;
    double x, y;  // Koordinaten für A* Heuristik
} Node;

// Struktur für die Kante zwischen zwei Knoten
typedef struct {
    int start;
    int end;
    double weight;
} Edge;

// Struktur für den Graphen
typedef struct {
    Node nodes[MAX_NODES];
    double adj_matrix[MAX_NODES][MAX_NODES];
    int num_nodes;
} Graph;

// Struktur für die Priority Queue
typedef struct {
    int node;
    double priority;
} PQItem;

typedef struct {
    PQItem* items;
    int capacity;
    int size;
} PriorityQueue;

// Priority Queue Funktionen
PriorityQueue* create_priority_queue(int capacity) {
    PriorityQueue* pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
    pq->items = (PQItem*)malloc(capacity * sizeof(PQItem));
    pq->capacity = capacity;
    pq->size = 0;
    return pq;
}

void swap(PQItem* a, PQItem* b) {
    PQItem temp = *a;
    *a = *b;
    *b = temp;
}

void heapify_up(PriorityQueue* pq, int idx) {
    while (idx > 0) {
        int parent = (idx - 1) / 2;
        if (pq->items[idx].priority < pq->items[parent].priority) {
            swap(&pq->items[idx], &pq->items[parent]);
            idx = parent;
        } else {
            break;
        }
    }
}

void push(PriorityQueue* pq, int node, double priority) {
    if (pq->size >= pq->capacity) {
        return;
    }
    
    pq->items[pq->size].node = node;
    pq->items[pq->size].priority = priority;
    heapify_up(pq, pq->size);
    pq->size++;
}

void heapify_down(PriorityQueue* pq, int idx) {
    while (1) {
        int smallest = idx;
        int left = 2 * idx + 1;
        int right = 2 * idx + 2;

        if (left < pq->size && pq->items[left].priority < pq->items[smallest].priority) {
            smallest = left;
        }
        if (right < pq->size && pq->items[right].priority < pq->items[smallest].priority) {
            smallest = right;
        }

        if (smallest == idx) {
            break;
        }

        swap(&pq->items[idx], &pq->items[smallest]);
        idx = smallest;
    }
}

PQItem pop(PriorityQueue* pq) {
    PQItem top = pq->items[0];
    pq->items[0] = pq->items[pq->size - 1];
    pq->size--;
    heapify_down(pq, 0);
    return top;
}

// Graph Funktionen
Graph* create_graph(int num_nodes) {
    Graph* graph = (Graph*)malloc(sizeof(Graph));
    graph->num_nodes = num_nodes;
    
    // Initialisiere Adjazenzmatrix mit Unendlich
    for (int i = 0; i < num_nodes; i++) {
        for (int j = 0; j < num_nodes; j++) {
            graph->adj_matrix[i][j] = INF;
        }
        graph->adj_matrix[i][i] = 0;
    }
    
    return graph;
}

void add_edge(Graph* graph, int start, int end, double weight) {
    graph->adj_matrix[start][end] = weight;
    graph->adj_matrix[end][start] = weight;  // Für ungerichteten Graphen
}

// Heuristik-Funktion für A* (Euklidische Distanz)
double heuristic(Node* a, Node* b) {
    double dx = a->x - b->x;
    double dy = a->y - b->y;
    return sqrt(dx * dx + dy * dy);
}

// Dijkstra Algorithmus
double* dijkstra(Graph* graph, int start, int* prev) {
    double* dist = (double*)malloc(graph->num_nodes * sizeof(double));
    int* visited = (int*)malloc(graph->num_nodes * sizeof(int));
    
    for (int i = 0; i < graph->num_nodes; i++) {
        dist[i] = INF;
        visited[i] = 0;
        prev[i] = -1;
    }
    
    dist[start] = 0;
    PriorityQueue* pq = create_priority_queue(graph->num_nodes);
    push(pq, start, 0);
    
    while (pq->size > 0) {
        PQItem current = pop(pq);
        int u = current.node;
        
        if (visited[u]) continue;
        visited[u] = 1;
        
        for (int v = 0; v < graph->num_nodes; v++) {
            if (graph->adj_matrix[u][v] != INF) {
                double alt = dist[u] + graph->adj_matrix[u][v];
                if (alt < dist[v]) {
                    dist[v] = alt;
                    prev[v] = u;
                    push(pq, v, alt);
                }
            }
        }
    }
    
    free(visited);
    free(pq->items);
    free(pq);
    return dist;
}

// A* Algorithmus
double* astar(Graph* graph, int start, int end, int* prev) {
    double* dist = (double*)malloc(graph->num_nodes * sizeof(double));
    double* f_score = (double*)malloc(graph->num_nodes * sizeof(double));
    int* visited = (int*)malloc(graph->num_nodes * sizeof(int));
    
    for (int i = 0; i < graph->num_nodes; i++) {
        dist[i] = INF;
        f_score[i] = INF;
        visited[i] = 0;
        prev[i] = -1;
    }
    
    dist[start] = 0;
    f_score[start] = heuristic(&graph->nodes[start], &graph->nodes[end]);
    
    PriorityQueue* pq = create_priority_queue(graph->num_nodes);
    push(pq, start, f_score[start]);
    
    while (pq->size > 0) {
        PQItem current = pop(pq);
        int u = current.node;
        
        if (u == end) break;
        if (visited[u]) continue;
        visited[u] = 1;
        
        for (int v = 0; v < graph->num_nodes; v++) {
            if (graph->adj_matrix[u][v] != INF) {
                double alt = dist[u] + graph->adj_matrix[u][v];
                if (alt < dist[v]) {
                    dist[v] = alt;
                    prev[v] = u;
                    f_score[v] = dist[v] + heuristic(&graph->nodes[v], &graph->nodes[end]);
                    push(pq, v, f_score[v]);
                }
            }
        }
    }
    
    free(visited);
    free(f_score);
    free(pq->items);
    free(pq);
    return dist;
}

// Hilfsfunktion zum Generieren eines zufälligen Testgraphen
Graph* generate_test_graph(int num_nodes, double density) {
    Graph* graph = create_graph(num_nodes);
    srand(time(NULL));
    
    // Generiere zufällige Koordinaten für die Knoten
    for (int i = 0; i < num_nodes; i++) {
        graph->nodes[i].id = i;
        graph->nodes[i].x = (double)(rand() % 1000);
        graph->nodes[i].y = (double)(rand() % 1000);
    }
    
    // Generiere zufällige Kanten
    for (int i = 0; i < num_nodes; i++) {
        for (int j = i + 1; j < num_nodes; j++) {
            if ((double)rand() / RAND_MAX < density) {
                double weight = sqrt(
                    pow(graph->nodes[i].x - graph->nodes[j].x, 2) +
                    pow(graph->nodes[i].y - graph->nodes[j].y, 2)
                );
                add_edge(graph, i, j, weight);
            }
        }
    }
    
    return graph;
}

// Hauptfunktion zum Testen und Vergleichen der Algorithmen
void compare_algorithms(Graph* graph, int start, int end) {
    int* prev_dijkstra = (int*)malloc(graph->num_nodes * sizeof(int));
    int* prev_astar = (int*)malloc(graph->num_nodes * sizeof(int));
    
    // Zeitmessung für Dijkstra
    clock_t start_time = clock();
    double* dist_dijkstra = dijkstra(graph, start, prev_dijkstra);
    double dijkstra_time = (double)(clock() - start_time) / CLOCKS_PER_SEC;
    
    // Zeitmessung für A*
    start_time = clock();
    double* dist_astar = astar(graph, start, end, prev_astar);
    double astar_time = (double)(clock() - start_time) / CLOCKS_PER_SEC;
    
    printf("\nVergleich der Algorithmen:\n");
    printf("Dijkstra Zeit: %f Sekunden\n", dijkstra_time);
    printf("A* Zeit: %f Sekunden\n", astar_time);
    printf("Dijkstra Distanz zum Ziel: %f\n", dist_dijkstra[end]);
    printf("A* Distanz zum Ziel: %f\n", dist_astar[end]);
    
    free(prev_dijkstra);
    free(prev_astar);
    free(dist_dijkstra);
    free(dist_astar);
}

int main() {
    // Teste verschiedene Szenarien
    int test_sizes[] = {100, 500, 1000};
    double test_densities[] = {0.1, 0.3, 0.5};
    
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            int size = test_sizes[i];
            double density = test_densities[j];
            
            printf("\nTest mit %d Knoten und %.1f Dichte:\n", size, density);
            Graph* test_graph = generate_test_graph(size, density);
            compare_algorithms(test_graph, 0, size-1);
            
            // Aufräumen
            free(test_graph);
        }
    }
    
    return 0;
}