import requests
from bs4 import BeautifulSoup
import sqlite3
from collections import defaultdict
import time

# Konfiguration der Datenbank
DB_NAME = "graph.db"
CREATE_TABLE = """
    CREATE TABLE IF NOT EXISTS edges (
        source text,
        target text
    );
"""

class Webcrawler:
    def __init__(self, start_url, max_depth=2):
        self.start_url = start_url
        self.max_depth = max_depth
        self.graph = defaultdict(list)
        self.crawler_queue = [(start_url, 0)]

    def crawl(self):
        while self.crawler_queue:
            url, depth = self.crawler_queue.pop(0)

            # Verwenden Sie einen User-Agent für die Anfrage
            headers = {"User-Agent": "Mozilla/5.0"}
            response = requests.get(url, headers=headers)
            response.raise_for_status()

            # Parsen des HTML-Contentes
            soup = BeautifulSoup(response.content, "html.parser")

            # Extrahieren von Links
            links = [a["href"] for a in soup.find_all("a") if a.has_attr("href")]

            # Vermeiden Sie die Überprüfung der gleichen Seite mehrmals
            if url in self.crawler_queue:
                continue

            # Füge die gefundenen Links zur Datenbank hinzu
            for link in links:
                if 0 < depth <= self.max_depth and link not in self.graph[url]:
                    self.graph[url].append(link)
                    self.crawler_queue.append((link, depth + 1))

            # Speichern des Ergebnisses in der Datenbank
            conn = sqlite3.connect(DB_NAME)
            c = conn.cursor()
            for edge in self.graph:
                c.execute(CREATE_TABLE)
                c.execute("INSERT INTO edges VALUES (?, ?)", (edge, ", ".join(self.graph[edge])))
            conn.commit()
            conn.close()

    def rate_limiting(self):
        # Verwenden Sie einen Epooldown für die Anfragen
        import time
        last_crawl_time = time.time() - 10  # Beispiel-Epoche für eine Überprüfung

        while True:
            current_time = time.time()
            if current_time - last_crawl_time > 10:  # Überprüfen Sie den Epocheonstint
                last_crawl_time = current_time
                for url in self.crawler_queue[:]:
                    self.crawler_queue.remove(url)

    def start(self):
        self.rate_limiting()
        self.crawl()

crawler = Webcrawler("https://www.example.com")
crawler.start()