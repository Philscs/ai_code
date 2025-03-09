import networkx as nx
import matplotlib.pyplot as plt

# Create a directed graph
G = nx.DiGraph()

# Add nodes to the graph
G.add_node("A")
G.add_node("B")
G.add_node("C")
G.add_node("D")

# Add edges to represent dependencies
G.add_edge("A", "B")
G.add_edge("B", "C")
G.add_edge("C", "D")

# Visualize the graph
nx.draw(G, with_labels=True)
plt.show()

# Perform impact analysis
def perform_impact_analysis(graph, target_node):
    impacted_nodes = set()
    stack = [target_node]

    while stack:
        node = stack.pop()
        impacted_nodes.add(node)

        for neighbor in graph.predecessors(node):
            if neighbor not in impacted_nodes:
                stack.append(neighbor)

    return impacted_nodes

# Example usage of impact analysis
target = "D"
impacted = perform_impact_analysis(G, target)
print(f"The nodes impacted by {target} are: {impacted}")
