import re

class FuzzySearch:
    def __init__(self, texts):
        self.texts = texts
        self.search_cache = {}

    def _levenshtein_distance(self, s1, s2):
        """
        Berechnet die Levenshtein-Distanz zwischen zwei Strings.

        :param s1: Erster String.
        :param s2: Zweiter String.
        :return: Die Levenshtein-Distanz zwischen den beiden Strings.
        """
        m = len(s1) + 1
        n = len(s2) + 1

        d = [[0] * n for _ in range(m)]

        for i in range(m):
            d[i][0] = i
        for j in range(n):
            d[0][j] = j

        for i in range(1, m):
            for j in range(1, n):
                cost = 0 if s1[i - 1] == s2[j - 1] else 1
                d[i][j] = min(d[i - 1][j] + 1,
                              d[i][j - 1] + 1,
                              d[i - 1][j - 1] + cost)

        return d[m - 1][n - 1]

    def _fuzzy_search(self, query):
        """
        Durchführt einen fuzzy-Search nach einem gegebenen Query.

        :param query: Der zu durchsuchende String.
        :return: Die Liste der am häufigsten vorkommenden Wörter im durchgesuchten Text.
        """
        results = {}

        for text in self.texts:
            # Durchführen der Levenshtein-Distanz- Berechnung
            distance = self._levenshtein_distance(query, text)

            if (distance, query) not in self.search_cache:
                self.search_cache[(distance, query)] = []

            self.search_cache[(distance, query)].append(text)

        # Filtern der Ergebnisse
        max_distance = 2  # Maximaler Abstand für einen gültigen Suchergebnis
        best_results = {}

        for (distance, query), text_list in self.search_cache.items():
            if distance <= max_distance:
                for text in text_list:
                    # Entfernen des Punktsextens und Verwenden der RegEx-Modifikation
                    cleaned_text = re.sub(r'[^\w\s]', '', text).lower()
                    cleaned_query = re.sub(r'[^\w\s]', '', query).lower()

                    if cleaned_text not in best_results:
                        best_results[cleaned_text] = 0

                    # Steigern des Wörterausmasses
                    best_results[cleaned_text] += 1

        return list(best_results.keys())

    def search(self, query):
        """
        Durchführt einen fuzzy-Search nach einem gegebenen Query.

        :param query: Der zu durchsuchende String.
        :return: Die Liste der am häufigsten vorkommenden Wörter im durchgesuchten Text.
        """
        results = self._fuzzy_search(query)

        # DoS-Schutz bei komplexen Suchmustern
        if len(results) > 10000:
            raise ValueError("Zu viele Anfragen innerhalb eines kurzen Zeitraums. Bitte warten Sie einiger Sekunden.")

        return results

# Beispiel-Anwendung
texts = [
    "Der Herr ist es nicht, der mich verletzt hat.",
    "Ich denke, ich habe einen Fehl am Herz gesucht.",
    "Es gibt einen Unterschied zwischen einem richtigen und einem falschen Todesseufzer.",
    "Die Liebe ist wie ein feuergebrannter Wald.",
]

fuzzy_search = FuzzySearch(texts)
query = "ich"
results = fuzzy_search.search(query)

for result in results:
    print(result)
