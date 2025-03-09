import ast
import json
import configparser
import os
import subprocess
from sphinx import __version__ as sphinx_version

class CodeAnalyzer:
    def __init__(self, quellcodeordner):
        self.quellcodeordner = quellcodeordner
        self.imports = {}
        self.classes = {}

    def analyse(self):
        # Analysieren Sie den Quellcode
        with open(self.quellcodeordner, 'r') as file:
            for node in ast.walk(ast.parse(file.read())):
                if isinstance(node, ast.Import):
                    for alias in node.names:
                        self.imports[alias.name] = node.module

                elif isinstance(node, ast.ClassDef):
                    self.classes[node.name] = {}

    def erweitern(self):
        # Erweitern Sie die Klassen mit Informationen
        for class_name, klass in self.classes.items():
            with open(f'{self.quellcodeordner}/{class_name}.py', 'r') as file:
                code = file.read()
                # Code-Abhängigkeiten erkennen und speichern
                for line_number, line in enumerate(code.split('\n')):
                    if '@' + class_name + '.' in line or f'{class_name}.' in line:
                        variable = line.split('@' + class_name + '.')[-1].strip()
                        self.imports[variable] = None

    def exportiere(self):
        # Dokumentation exportieren
        with open(f'{self.outputordner}/index.html', 'w') as file:
            file.write('# API-Dokumentation\n')
            file.write('## Überblick\n')
            for class_name in self.classes.keys():
                file.write(f'\n### {class_name}\n')
                # Hier Ihre eigene Dokumentations-Logik einfügen

def generate_sphinx_documentation(quellcodeordner, outputordner):
    # Sphinx-Dokumentation erstellen
    project_dir = f'{outputordner}/project'
    sphinx_cmd = [ 'sphinx-apidoc', '-o', project_dir, quellcodeordner ]
    app.exit_status = 0

    command = ['python', '-m', 'sphinx-apidoc', '-o', project_dir, '--ext-autodoc', 
'--autodoc-type=python']
    result = subprocess.run(command, cwd=project_dir)

    if result.returncode != 0:
        raise RuntimeError(f'Dokumentation nicht erfolgreich erstellt')

    sphinx_cmd.extend(['--config-file', 'config.ini'])
    app.exit_status = 0

    command = ['python', '-m', 'sphinx-build', project_dir, outputordner]
    result = subprocess.run(command)

    if result.returncode != 0:
        raise RuntimeError(f'Dokumentation nicht erfolgreich erstellt')

def main():
    # Konfiguration und Initialisierung
    config_file_path = os.path.abspath('config.ini')
    with open(config_file_path, 'r') as config_file:
        config = configparser.ConfigParser()
        config.read(config_file)

    quellcodeordner = config.get('generierungsparameter', 'quellcodeordner')
    outputordner = config.get('generierungsparameter', 'outputordner')

    # System starten
    code_analyzer = CodeAnalyzer(quellcodeordner)
    code_analyzer.analyse()
    code_analyzer.erweitern()

    sphinx_cmd = [ 'python', '-m', 'sphinx-apidoc', '-o', f'{outputordner}/project', 
quellcodeordner ]
    app.exit_status = 0

    command = ['python', '-m', 'sphinx-apidoc', '-o', f'{outputordner}/project', '--ext-autodoc', 
'--autodoc-type=python']
    result = subprocess.run(command, cwd=f'{outputordner}/project')

    if result.returncode != 0:
        raise RuntimeError(f'Dokumentation nicht erfolgreich erstellt')

    sphinx_cmd.extend(['--config-file', 'config.ini'])
    app.exit_status = 0

    command = ['python', '-m', 'sphinx-build', f'{outputordner}/project', outputordner]
    result = subprocess.run(command)

    if result.returncode != 0:
        raise RuntimeError(f'Dokumentation nicht erfolgreich erstellt')

if __name__ == '__main__':
    main()