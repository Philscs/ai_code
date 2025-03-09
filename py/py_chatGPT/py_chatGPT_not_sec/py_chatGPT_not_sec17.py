import ast
import networkx as nx

class CodeAnalyzer(ast.NodeVisitor):
    def __init__(self):
        self.graph = nx.DiGraph()
        self.current_function = None

    def visit_FunctionDef(self, node):
        self.current_function = node.name
        self.graph.add_node(node.name, type="function", args=[arg.arg for arg in node.args.args])
        self.generic_visit(node)
        self.current_function = None

    def visit_Call(self, node):
        if self.current_function:
            called_function = self._get_function_name(node.func)
            if called_function:
                self.graph.add_edge(self.current_function, called_function)
        self.generic_visit(node)

    def _get_function_name(self, node):
        if isinstance(node, ast.Name):
            return node.id
        elif isinstance(node, ast.Attribute):
            return node.attr
        return None

def analyze_code(source_code):
    tree = ast.parse(source_code)
    analyzer = CodeAnalyzer()
    analyzer.visit(tree)
    return analyzer.graph

def generate_test_cases(graph):
    test_cases = []

    for function in graph.nodes:
        if graph.nodes[function]['type'] == 'function':
            test_case = {
                "function": function,
                "inputs": ["<input_placeholder>" for _ in graph.nodes[function]['args']],
                "expected_output": "<expected_output_placeholder>"
            }
            test_cases.append(test_case)

    return test_cases

def main():
    # Beispielcode
    code = """
    def add(a, b):
        return a + b

    def multiply(x, y):
        return x * y

    def compute(a, b, c):
        sum_result = add(a, b)
        return multiply(sum_result, c)
    """

    # Code analysieren und Abhängigkeitsgraph generieren
    graph = analyze_code(code)

    # Testfälle generieren
    test_cases = generate_test_cases(graph)

    # Ausgabe
    for test_case in test_cases:
        print(f"Test für Funktion {test_case['function']}:")
        print(f"  Eingaben: {test_case['inputs']}")
        print(f"  Erwartete Ausgabe: {test_case['expected_output']}")
        print()

if __name__ == "__main__":
    main()
