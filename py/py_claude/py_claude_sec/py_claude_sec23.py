from typing import List, Tuple
import time
from functools import lru_cache
import re

class FuzzySearcher:
    def __init__(self, max_pattern_length: int = 100, timeout_seconds: float = 1.0):
        self.max_pattern_length = max_pattern_length
        self.timeout_seconds = timeout_seconds
        self.text_corpus = []

    def add_text(self, text: str) -> None:
        """Fügt Text zum Suchkorpus hinzu"""
        self.text_corpus.append(text)

    @lru_cache(maxsize=1024)
    def levenshtein_distance(self, s1: str, s2: str) -> int:
        """
        Berechnet die Levenshtein-Distanz zwischen zwei Strings.
        Verwendet Caching für bessere Performance.
        """
        if len(s1) < len(s2):
            return self.levenshtein_distance(s2, s1)

        if len(s2) == 0:
            return len(s1)

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

    def search(self, pattern: str, threshold: float = 0.8) -> List[Tuple[str, float]]:
        """
        Führt eine Fuzzy-Suche durch und gibt Matches mit Ähnlichkeitswerten zurück.
        
        Args:
            pattern: Suchmuster
            threshold: Minimale Ähnlichkeit (0-1)
            
        Returns:
            Liste von Tupeln (gefundener_text, ähnlichkeit)
        """
        # DoS-Schutz: Überprüfung der Pattern-Länge
        if len(pattern) > self.max_pattern_length:
            raise ValueError(f"Suchmuster zu lang (max {self.max_pattern_length} Zeichen)")

        # Grundlegende Pattern-Validierung
        if not pattern or not re.match(r'^[\w\s\-.,]+$', pattern):
            raise ValueError("Ungültiges Suchmuster")

        start_time = time.time()
        results = []

        # Durchsuche den Korpus
        for text in self.text_corpus:
            # Timeout-Überprüfung
            if time.time() - start_time > self.timeout_seconds:
                raise TimeoutError("Suche hat das Zeitlimit überschritten")

            # Berechne Ähnlichkeit
            distance = self.levenshtein_distance(pattern.lower(), text.lower())
            max_length = max(len(pattern), len(text))
            similarity = 1 - (distance / max_length)

            if similarity >= threshold:
                results.append((text, similarity))

        # Sortiere nach Ähnlichkeit
        return sorted(results, key=lambda x: x[1], reverse=True)

def demo_fuzzy_search():
    """Demonstriert die Verwendung des FuzzySearcher"""
    # Initialisierung
    searcher = FuzzySearcher(max_pattern_length=50, timeout_seconds=2.0)
    
    # Füge Beispieltexte hinzu
    example_texts = [
        "Python Programmierung",
        "Machine Learning Basics",
        "Python Programming Guide",
        "Data Science mit Python",
        "Pythun Programmirung"  # Absichtliche Tippfehler
    ]
    
    for text in example_texts:
        searcher.add_text(text)
    
    try:
        # Führe Beispielsuchen durch
        pattern = "Python Programierung"
        results = searcher.search(pattern, threshold=0.7)
        
        print(f"Suchergebnisse für '{pattern}':")
        for text, similarity in results:
            print(f"Match: {text:<25} (Ähnlichkeit: {similarity:.2f})")
            
    except (ValueError, TimeoutError) as e:
        print(f"Fehler bei der Suche: {e}")

if __name__ == "__main__":
    demo_fuzzy_search()