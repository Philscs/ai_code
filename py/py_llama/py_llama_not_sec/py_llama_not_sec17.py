import ast
import networkx as nx
from unittest import TestCase

class Node:
    def __init__(self, name):
        self.name = name

class Edge:
    def __init__(self, node1, node2):
        self.node1 = node1
        self.node2 = node2

class CodeAnalyzer:
    def __init__(self, source_code):
        self.source_code = source_code

    def get_nodes(self):
        tree = ast.parse(self.source_code)
        nodes = []
        for node in ast.walk(tree):
            if isinstance(node, (ast.FunctionDef, ast.ClassDef)):
                nodes.append(Node(f"{node.name}"))
        return nodes

    def get_edges(self):
        G = nx.Graph()
        nodes = self.get_nodes()
        for i in range(len(nodes)):
            for j in range(i + 1, len(nodes)):
                if hasattr(nodes[i], 'name') and hasattr(nodes[j], 'name'):
                    if nodes[i].name == "main" or nodes[j].name == "main":
                        G.add_edge(nodes[i].name, nodes[j].name)
        return [Edge(node1.name, node2.name) for node1 in nodes for node2 in nodes if node1 != 
node2]

class TestGenerator:
    def __init__(self, analyzer):
        self.analyzer = analyzer

    def generate_test(self):
        edges = self.analyzer.get_edges()
        test_cases = []
        for edge in edges:
            print(f"Test case für Übergabe von {edge.node1.name} an {edge.node2.name}:")
            # Hier eingegebene Werte für die Testfälle
            test_cases.append((f"{edge.node1.name}=True, {edge.node2.name}=False"))
            test_cases.append((f"{edge.node1.name}=False, {edge.node2.name}=True"))
        return test_cases

class TestCaseGenerator(TestCase):
    def generate_test_cases(self):
        # Hier werden die aus den Testgeneratoren generierten Testfälle in eine Liste aufgenommen
        test_cases = []
        for i in range(10):  # Beispiel für 10 Testfälle
            analyzer = CodeAnalyzer("example.py")
            generator = TestGenerator(analyzer)
            new_test_cases = generator.generate_test()
            test_cases.extend(new_test_cases)
        return test_cases

# Beispielsourcecode:
# main() -> hello_world(), foo_bar()
def hello_world():
    pass

def foo_bar():
    pass

if __name__ == "__main__":
    case_generator = TestCaseGenerator()
    test_cases = case_generator.generate_test_cases()
    for i, (test_case) in enumerate(test_cases):
        print(f"Testfall {i+1}:")
        print(test_case)
