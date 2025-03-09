import requests
from bs4 import BeautifulSoup
from urllib.parse import urljoin, urlparse
import time
from datetime import datetime
from neo4j import GraphDatabase
import threading
from queue import Queue, PriorityQueue
import logging
from typing import Set, Dict, Optional
from dataclasses import dataclass
from enum import Enum

# Konfiguration für Logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)

class CrawlingStrategy(Enum):
    BREADTH_FIRST = "bfs"
    DEPTH_FIRST = "dfs"
    PRIORITY = "priority"

@dataclass
class Page:
    url: str
    priority: int = 0
    depth: int = 0

class RateLimiter:
    def __init__(self, requests_per_second: int):
        self.delay = 1.0 / requests_per_second
        self.last_request = {}
        self.lock = threading.Lock()

    def wait(self, domain: str):
        with self.lock:
            if domain in self.last_request:
                elapsed = time.time() - self.last_request[domain]
                if elapsed < self.delay:
                    time.sleep(self.delay - elapsed)
            self.last_request[domain] = time.time()

class GraphStorage:
    def __init__(self, uri: str, user: str, password: str):
        self.driver = GraphDatabase.driver(uri, auth=(user, password))

    def close(self):
        self.driver.close()

    def save_page(self, url: str, title: str, content: str):
        with self.driver.session() as session:
            session.run("""
                MERGE (p:Page {url: $url})
                SET p.title = $title,
                    p.content = $content,
                    p.last_crawled = datetime()
            """, url=url, title=title, content=content)

    def save_link(self, from_url: str, to_url: str):
        with self.driver.session() as session:
            session.run("""
                MATCH (p1:Page {url: $from_url})
                MERGE (p2:Page {url: $to_url})
                MERGE (p1)-[r:LINKS_TO]->(p2)
            """, from_url=from_url, to_url=to_url)

class WebCrawler:
    def __init__(
        self,
        start_urls: list[str],
        storage: GraphStorage,
        strategy: CrawlingStrategy = CrawlingStrategy.BREADTH_FIRST,
        max_depth: int = 3,
        requests_per_second: int = 2
    ):
        self.start_urls = start_urls
        self.storage = storage
        self.strategy = strategy
        self.max_depth = max_depth
        self.rate_limiter = RateLimiter(requests_per_second)
        
        # Verschiedene Queues für verschiedene Strategien
        self.queue: Queue[Page] = Queue() if strategy == CrawlingStrategy.BREADTH_FIRST else PriorityQueue()
        
        self.visited: Set[str] = set()
        self.session = requests.Session()
        self.session.headers.update({
            'User-Agent': 'Mozilla/5.0 (compatible; CustomCrawler/1.0; +http://example.com/bot)'
        })

    def extract_links(self, soup: BeautifulSoup, base_url: str) -> list[str]:
        links = []
        for anchor in soup.find_all('a', href=True):
            href = anchor['href']
            absolute_url = urljoin(base_url, href)
            # Nur HTTP(S) URLs
            if absolute_url.startswith(('http://', 'https://')):
                links.append(absolute_url)
        return links

    def calculate_priority(self, url: str, depth: int) -> int:
        # Beispiel-Priorisierung basierend auf Tiefe und Domain
        priority = depth * 10
        if "news" in url or "blog" in url:
            priority -= 5
        return priority

    def crawl_page(self, page: Page) -> None:
        if page.url in self.visited or page.depth > self.max_depth:
            return

        domain = urlparse(page.url).netloc
        self.rate_limiter.wait(domain)

        try:
            response = self.session.get(page.url, timeout=10)
            response.raise_for_status()
            self.visited.add(page.url)

            soup = BeautifulSoup(response.text, 'html.parser')
            title = soup.title.string if soup.title else ""
            
            # Speichere die Seite in der Graphdatenbank
            self.storage.save_page(page.url, title, response.text)

            # Extrahiere und verarbeite Links
            links = self.extract_links(soup, page.url)
            for link in links:
                self.storage.save_link(page.url, link)
                if link not in self.visited:
                    new_page = Page(
                        url=link,
                        depth=page.depth + 1,
                        priority=self.calculate_priority(link, page.depth + 1)
                    )
                    if self.strategy == CrawlingStrategy.PRIORITY:
                        self.queue.put((new_page.priority, new_page))
                    else:
                        self.queue.put(new_page)

        except Exception as e:
            logging.error(f"Fehler beim Crawlen von {page.url}: {str(e)}")

    def run(self):
        # Initialisiere die Queue mit Start-URLs
        for url in self.start_urls:
            initial_page = Page(url=url, depth=0)
            if self.strategy == CrawlingStrategy.PRIORITY:
                priority = self.calculate_priority(url, 0)
                self.queue.put((priority, initial_page))
            else:
                self.queue.put(initial_page)

        while not self.queue.empty():
            if self.strategy == CrawlingStrategy.PRIORITY:
                _, page = self.queue.get()
            else:
                page = self.queue.get()
            
            self.crawl_page(page)

# Beispiel-Verwendung
if __name__ == "__main__":
    # Konfiguration
    neo4j_config = {
        "uri": "bolt://localhost:7687",
        "user": "neo4j",
        "password": "password"
    }
    
    start_urls = [
        "https://example.com",
        "https://example.org"
    ]
    
    # Initialisierung
    storage = GraphStorage(**neo4j_config)
    
    # Crawler mit verschiedenen Strategien
    crawler = WebCrawler(
        start_urls=start_urls,
        storage=storage,
        strategy=CrawlingStrategy.PRIORITY,
        max_depth=3,
        requests_per_second=2
    )
    
    # Start des Crawling-Prozesses
    try:
        crawler.run()
    finally:
        storage.close()