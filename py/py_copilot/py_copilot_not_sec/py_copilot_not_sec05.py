import requests
from bs4 import BeautifulSoup
from py2neo import Graph

# Initialize the graph database
graph = Graph("bolt://localhost:7687", auth=("username", "password"))

# Set the maximum depth for recursive crawling
MAX_DEPTH = 3

# Set the rate limit for making requests
RATE_LIMIT = 1  # 1 second

# Set the base URL to start crawling from
BASE_URL = "https://example.com"

# Set the visited URLs to avoid revisiting
visited_urls = set()

def crawl(url, depth=0):
    # Check if the maximum depth has been reached
    if depth > MAX_DEPTH:
        return

    # Check if the URL has already been visited
    if url in visited_urls:
        return

    # Add the URL to the visited set
    visited_urls.add(url)

    # Make a request to the URL
    response = requests.get(url)

    # Extract the links from the response
    soup = BeautifulSoup(response.text, "html.parser")
    links = soup.find_all("a")

    # Store the data in the graph database
    graph.run("CREATE (page:Page {url: $url})", url=url)
    for link in links:
        href = link.get("href")
        if href:
            graph.run("CREATE (page)-[:LINKS_TO]->(:Page {url: $url})", url=href)

    # Recursively crawl the extracted links
    for link in links:
        href = link.get("href")
        if href:
            crawl(href, depth + 1)

    # Wait for the rate limit before making the next request
    time.sleep(RATE_LIMIT)

# Start crawling from the base URL
crawl(BASE_URL)
