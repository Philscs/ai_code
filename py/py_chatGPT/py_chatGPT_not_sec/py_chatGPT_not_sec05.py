import time
import requests
from bs4 import BeautifulSoup
from collections import deque
from neo4j import GraphDatabase

class WebCrawler:
    def __init__(self, start_url, strategy="bfs", rate_limit=1):
        self.start_url = start_url
        self.visited = set()
        self.strategy = strategy
        self.rate_limit = rate_limit
        self.driver = GraphDatabase.driver("bolt://localhost:7687", auth=("neo4j", "password"))

    def save_to_graph(self, from_url, to_url):
        with self.driver.session() as session:
            session.run(
                "MERGE (a:Page {url: $from_url}) "
                "MERGE (b:Page {url: $to_url}) "
                "MERGE (a)-[:LINKS_TO]->(b)",
                from_url=from_url, to_url=to_url
            )

    def fetch_links(self, url):
        try:
            response = requests.get(url, timeout=5)
            response.raise_for_status()
            soup = BeautifulSoup(response.text, "html.parser")
            links = set()
            for a_tag in soup.find_all("a", href=True):
                link = a_tag["href"]
                if link.startswith("http"):
                    links.add(link)
            return links
        except requests.RequestException:
            return set()

    def crawl(self, max_pages=100):
        queue = deque([self.start_url]) if self.strategy == "bfs" else [self.start_url]
        while queue and len(self.visited) < max_pages:
            url = queue.popleft() if self.strategy == "bfs" else queue.pop()
            if url not in self.visited:
                print(f"Visiting: {url}")
                self.visited.add(url)

                # Extract and process links
                links = self.fetch_links(url)
                for link in links:
                    self.save_to_graph(url, link)
                    if link not in self.visited:
                        queue.append(link)

                # Apply rate limiting
                time.sleep(self.rate_limit)

    def close(self):
        self.driver.close()

if __name__ == "__main__":
    start_url = "https://example.com"
    crawler = WebCrawler(start_url, strategy="bfs", rate_limit=2)
    try:
        crawler.crawl(max_pages=50)
    finally:
        crawler.close()
