import re
from collections import Counter
import random

class CodeSmellDetector:
    def __init__(self, code):
        self.code = code

    def extract_code_smells(self):
        smells = []
        self._extract_long_method_smells(smells)
        self._extract_dead_code_smells(smells)
        self._extract_duplicate_code_smells(smells)
        return smells

    def _extract_long_method_smells(self, smells):
        lines = self.code.split('\n')
        method_lengths = [len(line) for line in lines if line.strip()]
        if len(method_lengths) > 1:
            max_length = max(method_lengths)
            if any(len(line) == max_length for line in lines):
                smells.append('Long Method')

    def _extract_dead_code_smells(self, smells):
        lines = self.code.split('\n')
        for i, line in enumerate(lines):
            cleaned_line = re.sub(r'\s+', '', line.strip())
            if not cleaned_line:
                smells.append('Dead Code')

    def _extract_duplicate_code_smells(self, smells):
        lines = self.code.split('\n')
        code_counts = Counter(line.strip() for line in lines)
        if any(count > 1 for count in code_counts.values()):
            smells.append('Duplicate Code')

class RefactoringSuggester:
    def __init__(self, smells):
        self.smells = smells

    def suggest_refactoring(self):
        suggestions = []
        for smell in self.smells:
            if smell == 'Long Method':
                suggestion = f'Kürzen der Methode um {random.randint(1, 5)} Zeilen'
                suggestions.append(suggestion)
            elif smell == 'Dead Code':
                suggestion = 'Entfernen von unnötigem Code'
                suggestions.append(suggestion)
            elif smell == 'Duplicate Code':
                suggestion = 'Stellen Sie sicher, dass gleicher Code nur einmal implementiert wird.'
                suggestions.append(suggestion)
        return suggestions

# Beispielanwendung
code = """
def long_method():
    for i in range(10):
        pass

if __name__ == "__main__":
    dead_code()
"""

detector = CodeSmellDetector(code)
smells = detector.extract_code_smells()

suggester = RefactoringSuggester(smells)
suggestions = suggester.suggest_refactoring()

for suggestion in suggestions:
    print(suggestion)