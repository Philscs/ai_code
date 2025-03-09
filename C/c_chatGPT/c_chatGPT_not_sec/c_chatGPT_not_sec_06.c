#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <time.h>

#define INF INT_MAX

typedef struct {
    int id;
    int x, y; // Koordinaten für A*
} Node;

typedef struct {
    int from;
    int to;
    int weight;
} Edge;

typedef struct {
    int n_nodes;
    int n_edges;
    Node *nodes;
    Edge *edges;
} Graph;

Graph *create_graph(int n_nodes, int n_edges) {
    Graph *g = (Graph *)malloc(sizeof(Graph));
    g->n_nodes = n_nodes;
    g->n_edges = n_edges;
    g->nodes = (Node *)malloc(n_nodes * sizeof(Node));
    g->edges = (Edge *)malloc(n_edges * sizeof(Edge));
    return g;
}

void free_graph(Graph *g) {
    free(g->nodes);
    free(g->edges);
    free(g);
}

int dijkstra(Graph *g, int start, int end) {
    int *dist = (int *)malloc(g->n_nodes * sizeof(int));
    int *visited = (int *)calloc(g->n_nodes, sizeof(int));

    for (int i = 0; i < g->n_nodes; i++) dist[i] = INF;
    dist[start] = 0;

    for (int i = 0; i < g->n_nodes; i++) {
        int u = -1;
        for (int j = 0; j < g->n_nodes; j++) {
            if (!visited[j] && (u == -1 || dist[j] < dist[u])) {
                u = j;
            }
        }

        if (dist[u] == INF) break;

        visited[u] = 1;

        for (int e = 0; e < g->n_edges; e++) {
            if (g->edges[e].from == u) {
                int v = g->edges[e].to;
                int weight = g->edges[e].weight;
                if (dist[u] + weight < dist[v]) {
                    dist[v] = dist[u] + weight;
                }
            }
        }
    }

    int result = dist[end];
    free(dist);
    free(visited);
    return result;
}

int heuristic(Node a, Node b) {
    return abs(a.x - b.x) + abs(a.y - b.y);
}

int a_star(Graph *g, int start, int end) {
    int *dist = (int *)malloc(g->n_nodes * sizeof(int));
    int *visited = (int *)calloc(g->n_nodes, sizeof(int));

    for (int i = 0; i < g->n_nodes; i++) dist[i] = INF;
    dist[start] = 0;

    while (1) {
        int u = -1;
        for (int i = 0; i < g->n_nodes; i++) {
            if (!visited[i] && (u == -1 || dist[i] + heuristic(g->nodes[i], g->nodes[end]) < dist[u] + heuristic(g->nodes[u], g->nodes[end]))) {
                u = i;
            }
        }

        if (u == -1 || u == end) break;
        visited[u] = 1;

        for (int e = 0; e < g->n_edges; e++) {
            if (g->edges[e].from == u) {
                int v = g->edges[e].to;
                int weight = g->edges[e].weight;
                if (dist[u] + weight < dist[v]) {
                    dist[v] = dist[u] + weight;
                }
            }
        }
    }

    int result = dist[end];
    free(dist);
    free(visited);
    return result;
}

void performance_test(Graph *g, int start, int end) {
    clock_t start_time, end_time;
    double time_taken;

    start_time = clock();
    int dijkstra_result = dijkstra(g, start, end);
    end_time = clock();
    time_taken = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Dijkstra: Result = %d, Time = %f seconds\n", dijkstra_result, time_taken);

    start_time = clock();
    int a_star_result = a_star(g, start, end);
    end_time = clock();
    time_taken = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("A*: Result = %d, Time = %f seconds\n", a_star_result, time_taken);
}

int main() {
    int n_nodes = 5;
    int n_edges = 7;

    Graph *g = create_graph(n_nodes, n_edges);

    g->nodes[0] = (Node){0, 0, 0};
    g->nodes[1] = (Node){1, 1, 0};
    g->nodes[2] = (Node){2, 1, 1};
    g->nodes[3] = (Node){3, 0, 1};
    g->nodes[4] = (Node){4, 2, 2};

    g->edges[0] = (Edge){0, 1, 1};
    g->edges[1] = (Edge){1, 2, 2};
    g->edges[2] = (Edge){2, 4, 1};
    g->edges[3] = (Edge){0, 3, 4};
    g->edges[4] = (Edge){3, 2, 1};
    g->edges[5] = (Edge){3, 4, 5};
    g->edges[6] = (Edge){1, 3, 3};

    performance_test(g, 0, 4);

    free_graph(g);
    return 0;
}
