import ast
import os
import inspect
from typing import Dict, List, Optional

class CodeDocumentationGenerator:
    def __init__(self, source_path: str, output_format: str = 'markdown'):
        self.source_path = source_path
        self.output_format = output_format.lower()
        self.supported_formats = ['markdown', 'html', 'json']

        if self.output_format not in self.supported_formats:
            raise ValueError(f"Unsupported format: {self.output_format}. Supported formats are: {self.supported_formats}")

    def analyze_code(self) -> Dict[str, List[str]]:
        dependencies = []
        functions = []
        classes = []

        for root, _, files in os.walk(self.source_path):
            for file in files:
                if file.endswith('.py'):
                    file_path = os.path.join(root, file)
                    with open(file_path, 'r', encoding='utf-8') as f:
                        content = f.read()

                    tree = ast.parse(content, filename=file_path)

                    for node in ast.walk(tree):
                        if isinstance(node, ast.Import):
                            for alias in node.names:
                                dependencies.append(alias.name)
                        elif isinstance(node, ast.ImportFrom):
                            module = node.module if node.module else ''
                            for alias in node.names:
                                dependencies.append(f"{module}.{alias.name}")
                        elif isinstance(node, ast.FunctionDef):
                            functions.append(node.name)
                        elif isinstance(node, ast.ClassDef):
                            classes.append(node.name)

        return {
            'dependencies': sorted(set(dependencies)),
            'functions': sorted(set(functions)),
            'classes': sorted(set(classes))
        }

    def generate_documentation(self) -> str:
        analysis = self.analyze_code()

        if self.output_format == 'markdown':
            return self._generate_markdown(analysis)
        elif self.output_format == 'html':
            return self._generate_html(analysis)
        elif self.output_format == 'json':
            return self._generate_json(analysis)

    def _generate_markdown(self, analysis: Dict[str, List[str]]) -> str:
        doc = "# Code Documentation\n\n"
        doc += "## Dependencies\n\n" + '\n'.join(f"- {dep}" for dep in analysis['dependencies']) + "\n\n"
        doc += "## Functions\n\n" + '\n'.join(f"- {func}" for func in analysis['functions']) + "\n\n"
        doc += "## Classes\n\n" + '\n'.join(f"- {cls}" for cls in analysis['classes']) + "\n"
        return doc

    def _generate_html(self, analysis: Dict[str, List[str]]) -> str:
        doc = "<html><body>\n"
        doc += "<h1>Code Documentation</h1>\n"
        doc += "<h2>Dependencies</h2><ul>" + ''.join(f"<li>{dep}</li>" for dep in analysis['dependencies']) + "</ul>\n"
        doc += "<h2>Functions</h2><ul>" + ''.join(f"<li>{func}</li>" for func in analysis['functions']) + "</ul>\n"
        doc += "<h2>Classes</h2><ul>" + ''.join(f"<li>{cls}</li>" for cls in analysis['classes']) + "</ul>\n"
        doc += "</body></html>"
        return doc

    def _generate_json(self, analysis: Dict[str, List[str]]) -> str:
        import json
        return json.dumps(analysis, indent=4)

    def save_documentation(self, output_file: str):
        doc = self.generate_documentation()
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(doc)

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Generate documentation from source code.")
    parser.add_argument("source_path", type=str, help="Path to the source code directory.")
    parser.add_argument("output_file", type=str, help="File to save the generated documentation.")
    parser.add_argument("--format", type=str, default="markdown", choices=["markdown", "html", "json"], help="Format of the output documentation.")

    args = parser.parse_args()

    generator = CodeDocumentationGenerator(args.source_path, args.format)
    generator.save_documentation(args.output_file)

    print(f"Documentation saved to {args.output_file}")
