import ast
from jinja2 import Template

# UML-Diagramm als Python-Objekt
class Klass:
    def __init__(self, name):
        self.name = name

# Funktion zum Erstellen von Code
def code_generator(class_definition, template):
    # AST-Erstellung mit pyast
    tree = ast.parse(template)

    # Ersetzen der Klasse durch die im UML-Diagramm genannte Klassenname
    class_name = "Klass"

    # Erzeugen des Code-ASTs
    code_ast = ast.parse(
        template.replace("{{class}}", class_name),
    )

    return code_ast

# Funktion zum Generieren von Code aus dem UML-Diagramm
def generate_code(uml_class, class_definition):
    template = '''
from {{class}} import *

class {{class}}:
    def __init__(self):
        pass
    # TODO: Implementieren des Konstructors

    def foo(self):
        pass
'''

    # Erzeugen des Code
    code_ast = code_generator(class_definition, template)

    return ast.unparse(code_ast)
