import re
from difflib import SequenceMatcher
from collections import defaultdict
import time

class FuzzySearch:
    def __init__(self, max_distance=2, max_execution_time=1.0):
        """
        :param max_distance: Maximale erlaubte Levenshtein-Distanz
        :param max_execution_time: Maximale Ausführungszeit für die Suche (in Sekunden)
        """
        self.max_distance = max_distance
        self.max_execution_time = max_execution_time

    def levenshtein_distance(self, s1, s2):
        """Berechnet die Levenshtein-Distanz zwischen zwei Zeichenketten."""
        if len(s1) < len(s2):
            return self.levenshtein_distance(s2, s1)

        # Früher Abbruch
        if abs(len(s1) - len(s2)) > self.max_distance:
            return float('inf')

        previous_row = range(len(s2) + 1)
        for i, c1 in enumerate(s1):
            current_row = [i + 1]
            for j, c2 in enumerate(s2):
                insertions = previous_row[j + 1] + 1
                deletions = current_row[j] + 1
                substitutions = previous_row[j] + (c1 != c2)
                current_row.append(min(insertions, deletions, substitutions))
            previous_row = current_row

        return previous_row[-1]

    def search(self, text, pattern):
        """Führt eine Fuzzy-Suche auf dem gegebenen Text durch."""
        start_time = time.time()
        results = []
        tokens = text.split()

        for token in tokens:
            # DoS-Schutz: Begrenze die Suchzeit
            if time.time() - start_time > self.max_execution_time:
                print("Maximale Suchzeit überschritten. Abbruch.")
                break

            # Berechne die Ähnlichkeit
            distance = self.levenshtein_distance(token, pattern)
            if distance <= self.max_distance:
                similarity = 1 - distance / max(len(token), len(pattern))
                results.append((token, similarity))

        return sorted(results, key=lambda x: -x[1])

if __name__ == "__main__":
    # Beispieltext
    text = "Dies ist ein einfacher Beispieltext, um die Funktionalität zu demonstrieren."
    pattern = "Beispiel"

    fuzzy_search = FuzzySearch(max_distance=2, max_execution_time=0.5)
    results = fuzzy_search.search(text, pattern)

    print("Ergebnisse:")
    for match, similarity in results:
        print(f"Treffer: {match}, Ähnlichkeit: {similarity:.2f}")
