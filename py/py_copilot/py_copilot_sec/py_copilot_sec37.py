import time
from threading import Lock
from collections import defaultdict
import threading

class RateLimiter:
    def __init__(self, capacity, refill_rate, strategy="token_bucket"):
        self.capacity = capacity
        self.refill_rate = refill_rate
        self.tokens = capacity
        self.last_refill_time = time.time()
        self.lock = Lock()
        self.strategy = strategy
        self.buckets = defaultdict(lambda: {
            "tokens": capacity,
            "last_refill": time.time()
        })

    def consume(self, tokens):
        with self.lock:
            self.refill_tokens()
            if tokens <= self.tokens:
                self.tokens -= tokens
                return True
            else:
                return False

    def refill_tokens(self):
        current_time = time.time()
        time_elapsed = current_time - self.last_refill_time
        tokens_to_add = time_elapsed * self.refill_rate
        self.tokens = min(self.capacity, self.tokens + tokens_to_add)
        self.last_refill_time = current_time

    def allow_request(self, key):
        with self.lock:
            self._refill_tokens(key)
            bucket = self.buckets[key]

            if self.strategy == "token_bucket":
                if bucket["tokens"] >= 1:
                    bucket["tokens"] -= 1
                    return True
                return False

            elif self.strategy == "leaky_bucket":
                if bucket["tokens"] >= 1:
                    bucket["tokens"] -= 1
                    return True
                else:
                    return False

            else:
                raise ValueError("Unknown strategy: {}".format(self.strategy))

    def _refill_tokens(self, key):
        current_time = time.time()
        bucket = self.buckets[key]
        elapsed_time = current_time - bucket["last_refill"]
        new_tokens = elapsed_time * self.refill_rate

        bucket["tokens"] = min(self.capacity, bucket["tokens"] + new_tokens)
        bucket["last_refill"] = current_time

    def distributed_refill(self):
        while True:
            with self.lock:
                for key in self.buckets.keys():
                    self._refill_tokens(key)
            time.sleep(1)

# Beispielnutzung
def main():
    limiter = RateLimiter(capacity=5, refill_rate=1, strategy="token_bucket")

    def simulate_user_requests(user_id):
        for _ in range(10):
            if limiter.allow_request(user_id):
                print(f"Anfrage von Benutzer*in {user_id} erlaubt.")
            else:
                print(f"Anfrage von Benutzer*in {user_id} abgelehnt.")
            time.sleep(0.5)

    threads = [
        threading.Thread(target=simulate_user_requests, args=(f"user_{i}",))
        for i in range(3)
    ]

    for thread in threads:
        thread.start()

    for thread in threads:
        thread.join()

if __name__ == "__main__":
    main()
