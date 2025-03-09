import requests
import threading
from queue import Queue
from bs4 import BeautifulSoup
import json
import csv
import time
from urllib.robotparser import RobotFileParser
from fake_useragent import UserAgent
import logging
from datetime import datetime
import random

class ProxyRotator:
    def __init__(self, proxy_list):
        self.proxies = proxy_list
        self.current_index = 0
        self.lock = threading.Lock()
    
    def get_next_proxy(self):
        with self.lock:
            proxy = self.proxies[self.current_index]
            self.current_index = (self.current_index + 1) % len(self.proxies)
            return proxy

class WebScraper:
    def __init__(self, config):
        self.config = config
        self.queue = Queue()
        self.results = []
        self.results_lock = threading.Lock()
        self.proxy_rotator = ProxyRotator(config.get('proxies', []))
        self.user_agent = UserAgent()
        self.setup_logging()
        
    def setup_logging(self):
        logging.basicConfig(
            filename=f'scraper_{datetime.now().strftime("%Y%m%d_%H%M%S")}.log',
            level=logging.INFO,
            format='%(asctime)s - %(levelname)s - %(message)s'
        )
        
    def check_robots_txt(self, url):
        try:
            rp = RobotFileParser()
            parsed_url = requests.utils.urlparse(url)
            robots_url = f"{parsed_url.scheme}://{parsed_url.netloc}/robots.txt"
            rp.set_url(robots_url)
            rp.read()
            return rp.can_fetch(self.user_agent.random, url)
        except Exception as e:
            logging.warning(f"Could not check robots.txt: {e}")
            return True
            
    def get_headers(self):
        return {
            'User-Agent': self.user_agent.random,
            'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8',
            'Accept-Language': 'en-US,en;q=0.5',
            'Connection': 'keep-alive',
        }
        
    def make_request(self, url):
        if not self.check_robots_txt(url):
            logging.warning(f"URL not allowed by robots.txt: {url}")
            return None
            
        proxy = self.proxy_rotator.get_next_proxy()
        headers = self.get_headers()
        
        try:
            response = requests.get(
                url,
                headers=headers,
                proxies={'http': proxy, 'https': proxy} if proxy else None,
                timeout=self.config.get('timeout', 30)
            )
            response.raise_for_status()
            return response
        except requests.exceptions.RequestException as e:
            logging.error(f"Request failed for {url}: {e}")
            return None
            
    def process_dynamic_content(self, response):
        """
        Process dynamically loaded content using Selenium if needed
        """
        if self.config.get('use_selenium', False):
            from selenium import webdriver
            from selenium.webdriver.chrome.options import Options
            from selenium.webdriver.support.ui import WebDriverWait
            from selenium.webdriver.support import expected_conditions as EC
            
            chrome_options = Options()
            chrome_options.add_argument('--headless')
            chrome_options.add_argument('--no-sandbox')
            
            driver = webdriver.Chrome(options=chrome_options)
            try:
                driver.get(response.url)
                # Wait for dynamic content to load
                WebDriverWait(driver, 10).until(
                    EC.presence_of_element_located((self.config['dynamic_element_locator']))
                )
                return driver.page_source
            finally:
                driver.quit()
        return response.text
        
    def scrape_page(self, url):
        response = self.make_request(url)
        if not response:
            return
            
        content = self.process_dynamic_content(response)
        soup = BeautifulSoup(content, 'html.parser')
        
        # Extract data based on configuration
        data = {}
        for field, selector in self.config['selectors'].items():
            elements = soup.select(selector)
            data[field] = [el.get_text(strip=True) for el in elements]
            
        with self.results_lock:
            self.results.append(data)
            
        # Follow pagination if configured
        if self.config.get('pagination_selector'):
            next_page = soup.select_one(self.config['pagination_selector'])
            if next_page and 'href' in next_page.attrs:
                self.queue.put(next_page['href'])
                
    def worker(self):
        while True:
            try:
                url = self.queue.get_nowait()
            except Queue.Empty:
                break
                
            self.scrape_page(url)
            
            # Respect rate limiting
            time.sleep(random.uniform(
                self.config.get('min_delay', 1),
                self.config.get('max_delay', 3)
            ))
            
            self.queue.task_done()
            
    def export_results(self, format='json'):
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        if format == 'json':
            with open(f'results_{timestamp}.json', 'w', encoding='utf-8') as f:
                json.dump(self.results, f, ensure_ascii=False, indent=2)
                
        elif format == 'csv':
            if not self.results:
                return
                
            fields = self.results[0].keys()
            with open(f'results_{timestamp}.csv', 'w', newline='', encoding='utf-8') as f:
                writer = csv.DictWriter(f, fieldnames=fields)
                writer.writeheader()
                writer.writerows(self.results)
                
    def run(self, start_urls):
        # Initialize queue with start URLs
        for url in start_urls:
            self.queue.put(url)
            
        # Start worker threads
        threads = []
        for _ in range(self.config.get('num_threads', 4)):
            t = threading.Thread(target=self.worker)
            t.start()
            threads.append(t)
            
        # Wait for all threads to complete
        for t in threads:
            t.join()
            
        # Export results in specified format
        self.export_results(self.config.get('export_format', 'json'))

# Example usage
if __name__ == "__main__":
    config = {
        'proxies': [
            'http://proxy1.example.com:8080',
            'http://proxy2.example.com:8080'
        ],
        'selectors': {
            'title': 'h1.title',
            'price': 'span.price',
            'description': 'div.description'
        },
        'pagination_selector': 'a.next-page',
        'num_threads': 4,
        'min_delay': 1,
        'max_delay': 3,
        'timeout': 30,
        'use_selenium': True,
        'dynamic_element_locator': ('class name', 'dynamic-content'),
        'export_format': 'json'
    }
    
    scraper = WebScraper(config)
    scraper.run(['https://example.com/page1', 'https://example.com/page2'])