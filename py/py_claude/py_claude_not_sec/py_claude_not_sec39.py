import os
import sys
import subprocess
import json
from typing import List, Dict, Any
from dataclasses import dataclass
from pathlib import Path
import ast

@dataclass
class CodeIssue:
    """Repräsentiert ein gefundenes Problem im Code."""
    file: str
    line: int
    message: str
    severity: str
    rule_id: str

class CodeAnalyzer:
    """Hauptklasse für die statische Code-Analyse."""
    
    def __init__(self, project_path: str):
        self.project_path = Path(project_path)
        self.issues: List[CodeIssue] = []

    def run_pylint(self) -> List[CodeIssue]:
        """Führt Pylint aus und sammelt Style- und Qualitätsprobleme."""
        try:
            result = subprocess.run(
                ['pylint', '--output-format=json', str(self.project_path)],
                capture_output=True,
                text=True
            )
            
            if result.stdout:
                issues = json.loads(result.stdout)
                for issue in issues:
                    self.issues.append(CodeIssue(
                        file=issue['path'],
                        line=issue['line'],
                        message=issue['message'],
                        severity=issue['type'],
                        rule_id=issue['symbol']
                    ))
        except subprocess.CalledProcessError as e:
            print(f"Fehler beim Ausführen von Pylint: {e}")
        
        return self.issues

    def run_mypy(self) -> List[CodeIssue]:
        """Führt mypy für Typ-Checking aus."""
        try:
            result = subprocess.run(
                ['mypy', str(self.project_path), '--show-error-codes'],
                capture_output=True,
                text=True
            )
            
            for line in result.stdout.split('\n'):
                if line.strip():
                    # Mypy-Ausgabeformat: file:line: severity: message  [error-code]
                    parts = line.split(':', 3)
                    if len(parts) >= 3:
                        file, line_num, message = parts[:3]
                        error_code = message.split('[')[-1].rstrip(']') if '[' in message else 'mypy-error'
                        
                        self.issues.append(CodeIssue(
                            file=file,
                            line=int(line_num),
                            message=message.strip(),
                            severity='error',
                            rule_id=error_code
                        ))
        except subprocess.CalledProcessError as e:
            print(f"Fehler beim Ausführen von mypy: {e}")
        
        return self.issues

class ComplexityAnalyzer:
    """Analysiert die Code-Komplexität."""
    
    def __init__(self, code: str):
        self.code = code
        self.tree = ast.parse(code)

    def analyze_complexity(self) -> Dict[str, Any]:
        """Berechnet verschiedene Komplexitätsmetriken."""
        stats = {
            'lines_of_code': len(self.code.splitlines()),
            'number_of_functions': 0,
            'number_of_classes': 0,
            'cyclomatic_complexity': 0
        }
        
        for node in ast.walk(self.tree):
            if isinstance(node, ast.FunctionDef):
                stats['number_of_functions'] += 1
                stats['cyclomatic_complexity'] += self._calculate_complexity(node)
            elif isinstance(node, ast.ClassDef):
                stats['number_of_classes'] += 1
        
        return stats

    def _calculate_complexity(self, node: ast.AST) -> int:
        """Berechnet die zyklomatische Komplexität einer Funktion."""
        complexity = 1  # Basiswert
        
        for child in ast.walk(node):
            if isinstance(child, (ast.If, ast.While, ast.For, ast.Assert,
                                ast.Try, ast.ExceptHandler)):
                complexity += 1
            elif isinstance(child, ast.BoolOp):
                complexity += len(child.values) - 1
        
        return complexity

class CodeReviewSystem:
    """Hauptklasse, die alle Analyse-Komponenten koordiniert."""
    
    def __init__(self, project_path: str):
        self.project_path = Path(project_path)
        self.analyzer = CodeAnalyzer(project_path)
        self.issues: List[CodeIssue] = []

    def run_full_analysis(self) -> Dict[str, Any]:
        """Führt eine vollständige Code-Analyse durch."""
        # Style und Qualitätsprüfung
        self.issues.extend(self.analyzer.run_pylint())
        
        # Typ-Checking
        self.issues.extend(self.analyzer.run_mypy())
        
        # Komplexitätsanalyse für jede Python-Datei
        complexity_results = {}
        for file_path in self.project_path.rglob('*.py'):
            with open(file_path, 'r', encoding='utf-8') as f:
                code = f.read()
                complexity_analyzer = ComplexityAnalyzer(code)
                complexity_results[str(file_path)] = complexity_analyzer.analyze_complexity()

        # Generiere Bericht
        report = {
            'issues': [vars(issue) for issue in self.issues],
            'complexity_analysis': complexity_results,
            'summary': self._generate_summary()
        }
        
        return report

    def _generate_summary(self) -> Dict[str, Any]:
        """Erstellt eine Zusammenfassung der Analyse."""
        severity_counts = {
            'error': 0,
            'warning': 0,
            'info': 0
        }
        
        for issue in self.issues:
            severity_counts[issue.severity] += 1

        return {
            'total_issues': len(self.issues),
            'severity_distribution': severity_counts,
            'files_analyzed': len(set(issue.file for issue in self.issues))
        }

def main():
    """Hauptfunktion für die Kommandozeilennutzung."""
    if len(sys.argv) != 2:
        print("Verwendung: python code_review.py <projektpfad>")
        sys.exit(1)

    project_path = sys.argv[1]
    reviewer = CodeReviewSystem(project_path)
    report = reviewer.run_full_analysis()
    
    # Ausgabe des Berichts als formatiertes JSON
    print(json.dumps(report, indent=2, ensure_ascii=False))

if __name__ == "__main__":
    main()