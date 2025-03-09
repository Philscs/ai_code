import os
import time
import random
import json
import csv
from concurrent.futures import ThreadPoolExecutor
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.chrome.options import Options
from fake_useragent import UserAgent
from bs4 import BeautifulSoup
import requests
from itertools import cycle

# Proxy rotation
PROXIES = ["http://proxy1", "http://proxy2", "http://proxy3"]  # Replace with real proxies
proxy_pool = cycle(PROXIES)

# Headers rotation
USER_AGENT = UserAgent()
HEADERS = {"User-Agent": USER_AGENT.random}

# Initialize Selenium WebDriver
def get_webdriver():
    chrome_options = Options()
    chrome_options.add_argument("--headless")
    chrome_options.add_argument(f"--user-agent={USER_AGENT.random}")
    proxy = next(proxy_pool)
    chrome_options.add_argument(f"--proxy-server={proxy}")

    service = Service("/path/to/chromedriver")  # Replace with your ChromeDriver path
    return webdriver.Chrome(service=service, options=chrome_options)

# Function to scrape a single page
def scrape_page(url):
    try:
        # Respect robots.txt
        robots_url = f"{url.rstrip('/')}/robots.txt"
        response = requests.get(robots_url, headers=HEADERS)
        if response.status_code == 200 and "Disallow" in response.text:
            print(f"Skipping {url}, disallowed by robots.txt.")
            return None

        # Load dynamic content using Selenium
        driver = get_webdriver()
        driver.get(url)
        WebDriverWait(driver, 10).until(
            EC.presence_of_element_located((By.TAG_NAME, "body"))
        )
        soup = BeautifulSoup(driver.page_source, "html.parser")
        driver.quit()

        # Process content
        title = soup.title.string if soup.title else "No Title"
        text_content = soup.get_text(strip=True)

        return {"url": url, "title": title, "content": text_content}

    except Exception as e:
        print(f"Error scraping {url}: {e}")
        return None

# Export functions
def export_to_json(data, filename="output.json"):
    with open(filename, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=4)

def export_to_csv(data, filename="output.csv"):
    with open(filename, "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["url", "title", "content"])
        for entry in data:
            writer.writerow([entry["url"], entry["title"], entry["content"]])

# Main function
def main(urls, output_format="json"):
    scraped_data = []

    with ThreadPoolExecutor(max_workers=5) as executor:  # Adjust max_workers as needed
        results = executor.map(scrape_page, urls)

    for result in results:
        if result:
            scraped_data.append(result)

    if output_format == "json":
        export_to_json(scraped_data)
    elif output_format == "csv":
        export_to_csv(scraped_data)
    else:
        print("Unsupported format. Supported formats are 'json' and 'csv'.")

if __name__ == "__main__":
    # Replace with actual URLs to scrape
    urls_to_scrape = [
        "https://example.com",
        "https://example.org",
        "https://example.net"
    ]

    main(urls_to_scrape, output_format="json")
