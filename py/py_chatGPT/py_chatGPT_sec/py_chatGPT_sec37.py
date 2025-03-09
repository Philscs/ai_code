import time
import threading
from collections import defaultdict

class RateLimiter:
    def __init__(self, strategy="token_bucket", max_tokens=10, refill_rate=1):
        """
        Rate Limiter mit verschiedenen Strategien.

        :param strategy: "token_bucket" oder "leaky_bucket"
        :param max_tokens: Maximale Anzahl an Tokens im Bucket
        :param refill_rate: Anzahl der Tokens, die pro Sekunde nachgefüllt werden
        """
        self.strategy = strategy
        self.max_tokens = max_tokens
        self.refill_rate = refill_rate
        self.buckets = defaultdict(lambda: {
            "tokens": max_tokens,
            "last_refill": time.time()
        })
        self.lock = threading.Lock()

    def _refill_tokens(self, key):
        """Füllt Tokens im Bucket auf."""
        current_time = time.time()
        bucket = self.buckets[key]
        elapsed_time = current_time - bucket["last_refill"]
        new_tokens = elapsed_time * self.refill_rate

        # Begrenzen auf max_tokens
        bucket["tokens"] = min(self.max_tokens, bucket["tokens"] + new_tokens)
        bucket["last_refill"] = current_time

    def allow_request(self, key):
        """
        Überprüft, ob eine Anfrage zugelassen wird.

        :param key: Eindeutiger Schlüssel (z. B. Benutzer*innen-ID oder IP-Adresse)
        :return: True, wenn die Anfrage erlaubt ist, sonst False
        """
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
                raise ValueError("Unbekannte Strategie: {}".format(self.strategy))

    def distributed_refill(self):
        """Refill-Logik für verteiltes Rate Limiting."""
        while True:
            with self.lock:
                for key in self.buckets.keys():
                    self._refill_tokens(key)
            time.sleep(1)

# Beispielnutzung
def main():
    limiter = RateLimiter(strategy="token_bucket", max_tokens=5, refill_rate=1)

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
