import time
from collections import defaultdict, deque

class TokenBucket:
    def __init__(self, rate, capacity):
        """
        Initialize the token bucket with the given rate and capacity.
        
        :param rate: The rate at which tokens are added to the bucket (in Hz)
        :param capacity: The maximum number of tokens in the bucket
        """
        self.rate = rate
        self.capacity = capacity
        self.tokens = 0
        self.last_reset = time.time()

    def consume(self, amount):
        """
        Consume the given amount of tokens from the bucket.
        
        :param amount: The number of tokens to consume
        :return: True if enough tokens were consumed, False otherwise
        """
        now = time.time()
        elapsed = now - self.last_reset
        tokens_to_add = int(elapsed * self.rate)
        self.tokens = min(self.capacity, self.tokens + tokens_to_add)

        # If the bucket has not been reset yet, add all remaining capacity to it
        if self.tokens < amount:
            return False

        # Consume from the current token count
        consumed_tokens = min(amount, self.tokens - amount)
        self.tokens -= consumed_tokens

        # Reset time and tokens when the bucket is empty
        if consumed_tokens == amount:
            self.last_reset = now
        return True


class LeakyBucket:
    def __init__(self, rate):
        """
        Initialize the leaky bucket with the given rate.
        
        :param rate: The rate at which tokens are added to the bucket (in Hz)
        """
        self.rate = rate
        self.tokens = 0

    def consume(self, amount):
        """
        Consume the given amount of tokens from the bucket.
        
        :param amount: The number of tokens to consume
        :return: True if enough tokens were consumed, False otherwise
        """
        now = time.time()
        elapsed = now - self.last_reset
        tokens_to_add = int(elapsed * self.rate)
        self.tokens = min(self.capacity, self.tokens + tokens_to_add)

        # If the bucket has not been reset yet, add all remaining capacity to it
        if self.tokens < amount:
            return False

        # Consume from the current token count
        consumed_tokens = min(amount, self.tokens - amount)
        self.tokens -= consumed_tokens

        # Reset time and tokens when the bucket is empty
        if consumed_tokens == amount:
            self.last_reset = now
        return True


class RateLimiter:
    def __init__(self):
        """
        Initialize the rate limiter with default settings.
        
        The Token Bucket strategy uses a rate of 1 Hz and a capacity of 100 tokens. 
        The Leaky Bucket strategy also uses a rate of 1 Hz but no fixed capacity, 
        instead it's implemented as a continuous stream of tokens.
        """
        self.token_bucket = TokenBucket(rate=1, capacity=100)
        self.leaky_bucket = LeakyBucket(rate=1)

    def is_allowed(self):
        """
        Check if an IP address is allowed to make a request within the last 10 seconds.
        
        :return: True if the IP address is allowed, False otherwise
        """
        # Get the current timestamp in seconds since epoch
        now = time.time()
        elapsed = now - self.token_bucket.last_reset

        # If the token bucket has been reset, return True (allow access)
        if elapsed >= 10:
            return True

        # Check if the IP address is allowed to consume tokens from the bucket
        return self.token_bucket.consume(1)

    def get_token_amount(self):
        """
        Get the amount of tokens in the Leaky Bucket.
        
        :return: The number of tokens in the leaky bucket
        """
        return self.leaky_bucket.tokens


# Usage example:
limiter = RateLimiter()

def make_request(ip_address):
    if not limiter.is_allowed():
        print(f"IP address {ip_address} is rate limited, cannot make request.")
        return

    # Allow the IP address to make a request and update its access time
    print(f"IP address {ip_address} allowed to make request.")

# Example usage with different strategies:
if __name__ == "__main__":
    limiter = RateLimiter()

    def token_bucket_strategy(ip_address):
        if not limiter.token_bucket.consume(1):
            print(f"IP address {ip_address} is rate limited by Token Bucket strategy.")
            return
        print(f"IP address {ip_address} allowed to make request using Token Bucket strategy.")

    def leaky_bucket_strategy(ip_address):
        tokens = limiter.leaky_bucket.tokens
        if tokens > 0:
            # Allow the IP address to consume tokens and update its access time
            print(f"IP address {ip_address} has {tokens} tokens available in Leaky Bucket strategy.")
        else:
            print(f"IP address {ip_address} is rate limited by Leaky Bucket strategy.")

    make_request("192.168.1.1")  # Should allow the request
    limiter.token_bucket.consume(10)  # Allow the request to complete within 10 seconds
    time.sleep(11)
    make_request("192.168.1.1")  # Should block the request due to rate limiting

    make_request("192.168.1.2")  # Should allow the request
    leaky_bucket_strategy("192.168.1.2")  # Should show available tokens in Leaky Bucket strategy