import ast
from jinja2 import Template

class Klass:
    def __init__(self, name):
        self.name = name

def code_generator(class_definition, template):
    # AST-Erstellung mit pyast
    tree = ast.parse(template)

    # Ersetzen der Klasse durch die im UML-Diagramm genannte Klassenname
    class_name = class_definition.__name__

    # Erzeugen des Code-ASTs
    code_ast = ast.parse(
        template.replace("{{class}}", class_name),
    )

    return code_ast

def generate_code(uml_class, class_definition):
    template = '''
from {{class}} import *

class {{class}}:
    def __init__(self, name):
        self.name = name
        # TODO: Implementieren des Konstructors

    def foo(self):
        print("Hello, I'm a method!")

    def bar(self):
        print("Hello, I'm another method!")
'''

    # Erzeugen des Code
    code_ast = code_generator(class_definition, template)

    return ast.unparse(code_ast)
