import requests
from bs4 import BeautifulSoup
from concurrent.futures import ThreadPoolExecutor
import time
import random

# Einstellungen
PROXY_LISTE = [
    "http://proxy1:8080",
    "http://proxy2:8080",
    # Hinzufügen von Proxy-Liste-Einheiten
]
URLLISTE = ["https://example.com/", "https://example.org/"]
ERgebnisFORMAT = {
    "json": lambda x: {"url": x},
    "csv": lambda x: ", ".join([x["url"]]),
    "txt": lambda x: ", ".join([x["url"]])
}
BLOCK_TIME = 10 # Sekunden
RATIOS = [1, 2, 3]

class WebScraper:
    def __init__(self):
        self.proxies = {}
        self.threads = []
        self.result = {}

    def proxy_rotation(self):
        while True:
            for proxy in PROXY_LISTE:
                if proxy not in self.proxies:
                    self.proxies[proxy] = requests.Session()
                    break

    def scrape(self, url):
        print(f"Scraping {url}")
        try:
            response = requests.get(url, proxies=self.proxies)
            response.raise_for_status()

            soup = BeautifulSoup(response.text, 'html.parser')
            for link in soup.find_all('a'):
                href = link.get('href')
                if href and href.startswith("http"):
                    self.threads.append(self.scrape_url(href))

        except Exception as e:
            print(f"Fehler beim Scrapen von {url}: {str(e)}")

    def scrape_url(self, url):
        try:
            response = requests.get(url, proxies=self.proxies)
            response.raise_for_status()

            soup = BeautifulSoup(response.text, 'html.parser')
            # Hier wird das zu scrapende Inhalte in der Variable data gesammelt
            for link in soup.find_all('a'):
                href = link.get('href')
                if href and href.startswith("http"):
                    self.threads.append(self.scrape_url(href))
        except Exception as e:
            print(f"Fehler beim Scrapen von {url}: {str(e)}")

    def start_scraping(self):
        self.proxy_rotation()

        for ratio in RATIOS:
            thread = Thread(target=self.start_thread, args=(ratio,))
            self.threads.append(thread)
            thread.start()

        for _ in range(len(RATIOS)):
            time.sleep(BLOCK_TIME)

    def export_result(self, format):
        if format == "json":
            result_json = {key: value for key, value in self.result.items()}
            import json
            with open("result.json", "w") as f:
                json.dump(result_json, f)
        elif format == "csv":
            with open("result.csv", "w") as f:
                f.write(self.result["url"] + "\n")
        elif format == "txt":
            with open("result.txt", "w") as f:
                for url in self.result["url"]:
                    f.write(url + "\n")

    def start_thread(self, ratio):
        while True:
            for _ in range(ratio):
                URL = random.choice(URLLISTE)
                self.scrape(URL)

def main():
    web_scraper = WebScraper()
    web_scraper.start_scraping()

    time.sleep(60*10) # Warte 10 Minuten

    web_scraper.export_result("json")

if __name__ == "__main__":
    main()