import requests
import threading
import time
import random
from bs4 import BeautifulSoup
from urllib.parse import urljoin
import concurrent.futures

# Configuration
BASE_URL = "http://localhost:8080"  # Adjust to your server's address
INITIAL_PAGE = "/path/index2.html"  # The main page to request
NUM_THREADS = 40  # Number of concurrent threads
DURATION = 30  # Duration of the test in seconds
REQUEST_DELAY = 0.1  # Delay between requests in seconds

# Event to signal when to stop worker threads
stop_event = threading.Event()

def fetch_url(url):
    try:
        response = requests.get(url)
        return response.text
    except requests.RequestException as e:
        print(f"Error fetching {url}: {e}")
        return None

def get_referenced_resources(html, base_url):
    soup = BeautifulSoup(html, 'html.parser')
    resources = []

    # Find all <img>, <script>, and <link> tags
    for tag in soup.find_all(['img', 'script', 'link']):
        if tag.name == 'img' and tag.get('src'):
            resources.append(urljoin(base_url, tag['src']))
        elif tag.name == 'script' and tag.get('src'):
            resources.append(urljoin(base_url, tag['src']))
        elif tag.name == 'link' and tag.get('href'):
            resources.append(urljoin(base_url, tag['href']))

    return resources

def worker(results):
    requests_made = 0

    while not stop_event.is_set():
        # Fetch the main page
        main_url = BASE_URL + INITIAL_PAGE
        html = fetch_url(main_url)
        requests_made += 1

        if html:
            # Get referenced resources
            resources = get_referenced_resources(html, BASE_URL)

            # Fetch all resources concurrently
            with concurrent.futures.ThreadPoolExecutor(max_workers=10) as executor:
                future_to_url = {executor.submit(fetch_url, url): url for url in resources}
                for future in concurrent.futures.as_completed(future_to_url):
                    requests_made += 1

        # Delay between requests
        time.sleep(REQUEST_DELAY)

    results.append(requests_made)

def main():
    threads = []
    results = []

    # Start all worker threads
    for _ in range(NUM_THREADS):
        t = threading.Thread(target=worker, args=(results,))
        t.start()
        threads.append(t)
        # Random delay between 1 and 2 seconds before spawning the next thread
        time.sleep(random.uniform(1, 2))

    # Let the threads run for the specified duration
    time.sleep(DURATION)
    stop_event.set()

    # Wait for all threads to finish
    for t in threads:
        t.join()

    # Calculate total requests made
    total_requests = sum(results)
    print(f"Total requests made: {total_requests}")
    print(f"Requests per second: {total_requests / DURATION:.2f}")

if __name__ == "__main__":
    main()

