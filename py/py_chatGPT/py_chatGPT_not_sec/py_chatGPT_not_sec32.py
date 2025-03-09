import ast

class CodeSmellDetector(ast.NodeVisitor):
    def __init__(self):
        self.issues = []

    def visit_FunctionDef(self, node):
        # Long Method
        if len(node.body) > 20:
            self.issues.append({
                'type': 'Long Method',
                'message': f'Function "{node.name}" has too many lines ({len(node.body)}).',
                'line': node.lineno,
                'suggestion': 'Consider breaking this function into smaller, more focused functions.'
            })
        self.generic_visit(node)

    def visit_ClassDef(self, node):
        # Large Class
        if len(node.body) > 30:
            self.issues.append({
                'type': 'Large Class',
                'message': f'Class "{node.name}" has too many methods or attributes ({len(node.body)}).',
                'line': node.lineno,
                'suggestion': 'Consider splitting this class into smaller, more cohesive classes.'
            })
        self.generic_visit(node)

    def visit_Assign(self, node):
        # Magic Numbers
        if isinstance(node.value, ast.Constant) and isinstance(node.value.value, (int, float)):
            self.issues.append({
                'type': 'Magic Number',
                'message': f'Magic number found on line {node.lineno}.',
                'line': node.lineno,
                'suggestion': 'Consider replacing this magic number with a named constant for better readability.'
            })
        self.generic_visit(node)

    def visit_If(self, node):
        # Complex Conditionals
        if isinstance(node.test, ast.BoolOp) and len(node.test.values) > 3:
            self.issues.append({
                'type': 'Complex Conditional',
                'message': f'Complex conditional found on line {node.lineno}.',
                'line': node.lineno,
                'suggestion': 'Consider simplifying this conditional or breaking it into smaller conditions.'
            })
        self.generic_visit(node)

    def report(self):
        if not self.issues:
            print("No code smells detected.")
        else:
            print("Code Smells Detected:")
            for issue in self.issues:
                print(f"- {issue['type']} (line {issue['line']}): {issue['message']}")
                print(f"  Suggestion: {issue['suggestion']}\n")

if __name__ == "__main__":
    # Example Python code to analyze
    sample_code = """
class Example:
    def method_one(self):
        pass

    def method_two(self):
        for i in range(100):
            print(i)

class AnotherExample:
    def another_method(self):
        if True and False and True and True:
            print("Complex condition")

    def yet_another_method(self):
        x = 42
    """

    tree = ast.parse(sample_code)
    detector = CodeSmellDetector()
    detector.visit(tree)
    detector.report()
