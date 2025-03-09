from abc import ABC, abstractmethod
import time
import threading
import redis
from dataclasses import dataclass
from typing import Optional
from datetime import datetime

@dataclass
class RateLimitResult:
    """Ergebnis eines Rate Limit Checks"""
    is_allowed: bool
    remaining_tokens: int
    retry_after: Optional[float] = None

class RateLimiter(ABC):
    """Basis-Klasse für Rate Limiter Implementierungen"""
    
    @abstractmethod
    def try_acquire(self, key: str, tokens: int = 1) -> RateLimitResult:
        """Versucht Tokens zu akquirieren"""
        pass

class TokenBucketLimiter(RateLimiter):
    """Token Bucket Implementierung mit Redis-Backend für verteiltes Rate Limiting"""
    
    def __init__(
        self,
        redis_client: redis.Redis,
        bucket_capacity: int,
        refill_rate: float,
        window_size: int = 1
    ):
        self.redis = redis_client
        self.bucket_capacity = bucket_capacity
        self.refill_rate = refill_rate  # Tokens pro Sekunde
        self.window_size = window_size
        
    def try_acquire(self, key: str, tokens: int = 1) -> RateLimitResult:
        """
        Versucht Tokens aus dem Bucket zu entnehmen.
        
        Args:
            key: Eindeutiger Identifier für den Bucket (z.B. IP oder User-ID)
            tokens: Anzahl der benötigten Tokens
            
        Returns:
            RateLimitResult mit Status und verbleibenden Tokens
        """
        lua_script = """
        local key = KEYS[1]
        local tokens_needed = tonumber(ARGV[1])
        local bucket_capacity = tonumber(ARGV[2])
        local refill_rate = tonumber(ARGV[3])
        local now = tonumber(ARGV[4])
        local window_size = tonumber(ARGV[5])
        
        -- Get current bucket state
        local bucket = redis.call('hmget', key, 'tokens', 'last_update')
        local current_tokens = tonumber(bucket[1] or bucket_capacity)
        local last_update = tonumber(bucket[2] or now)
        
        -- Calculate token refill
        local time_passed = now - last_update
        local new_tokens = math.min(
            bucket_capacity,
            current_tokens + (time_passed * refill_rate)
        )
        
        -- Check if we have enough tokens
        if new_tokens >= tokens_needed then
            -- Update bucket
            new_tokens = new_tokens - tokens_needed
            redis.call('hmset', key, 'tokens', new_tokens, 'last_update', now)
            redis.call('expire', key, window_size)
            return {1, new_tokens, 0}  -- Allowed
        else
            -- Calculate retry after
            local retry_after = (tokens_needed - new_tokens) / refill_rate
            return {0, new_tokens, retry_after}  -- Not allowed
        end
        """
        
        # Führe Lua Script atomar aus
        script = self.redis.register_script(lua_script)
        result = script(
            keys=[key],
            args=[
                tokens,
                self.bucket_capacity,
                self.refill_rate,
                time.time(),
                self.window_size
            ]
        )
        
        return RateLimitResult(
            is_allowed=bool(result[0]),
            remaining_tokens=result[1],
            retry_after=result[2] if not bool(result[0]) else None
        )

class LeakyBucketLimiter(RateLimiter):
    """Leaky Bucket Implementierung mit Redis-Backend"""
    
    def __init__(
        self,
        redis_client: redis.Redis,
        bucket_capacity: int,
        leak_rate: float,
        window_size: int = 1
    ):
        self.redis = redis_client
        self.bucket_capacity = bucket_capacity
        self.leak_rate = leak_rate  # Requests pro Sekunde
        self.window_size = window_size
        
    def try_acquire(self, key: str, tokens: int = 1) -> RateLimitResult:
        lua_script = """
        local key = KEYS[1]
        local tokens = tonumber(ARGV[1])
        local capacity = tonumber(ARGV[2])
        local leak_rate = tonumber(ARGV[3])
        local now = tonumber(ARGV[4])
        local window_size = tonumber(ARGV[5])
        
        -- Get current water level
        local bucket = redis.call('hmget', key, 'water', 'last_leak')
        local current_water = tonumber(bucket[1] or 0)
        local last_leak = tonumber(bucket[2] or now)
        
        -- Calculate leakage
        local time_passed = now - last_leak
        local leaked = time_passed * leak_rate
        local new_water = math.max(0, current_water - leaked)
        
        -- Check if adding water exceeds capacity
        if (new_water + tokens) <= capacity then
            -- Update bucket
            redis.call('hmset', key, 'water', new_water + tokens, 'last_leak', now)
            redis.call('expire', key, window_size)
            return {1, capacity - (new_water + tokens), 0}  -- Allowed
        else
            -- Calculate retry after
            local retry_after = (new_water + tokens - capacity) / leak_rate
            return {0, capacity - new_water, retry_after}  -- Not allowed
        end
        """
        
        script = self.redis.register_script(lua_script)
        result = script(
            keys=[key],
            args=[
                tokens,
                self.bucket_capacity,
                self.leak_rate,
                time.time(),
                self.window_size
            ]
        )
        
        return RateLimitResult(
            is_allowed=bool(result[0]),
            remaining_tokens=result[1],
            retry_after=result[2] if not bool(result[0]) else None
        )

class DoSProtection:
    """DoS-Schutz Wrapper für Rate Limiter"""
    
    def __init__(
        self,
        rate_limiter: RateLimiter,
        max_concurrent: int = 100,
        ban_threshold: int = 10,
        ban_duration: int = 3600
    ):
        self.rate_limiter = rate_limiter
        self.max_concurrent = max_concurrent
        self.ban_threshold = ban_threshold
        self.ban_duration = ban_duration
        self.concurrent_requests = {}
        self.violation_counts = {}
        self.lock = threading.Lock()
        
    def is_banned(self, key: str) -> bool:
        """Prüft ob eine IP/User gebannt ist"""
        return self.violation_counts.get(key, 0) >= self.ban_threshold
        
    def try_acquire(self, key: str, tokens: int = 1) -> RateLimitResult:
        if self.is_banned(key):
            return RateLimitResult(
                is_allowed=False,
                remaining_tokens=0,
                retry_after=self.ban_duration
            )
            
        with self.lock:
            current = self.concurrent_requests.get(key, 0)
            if current >= self.max_concurrent:
                self.violation_counts[key] = self.violation_counts.get(key, 0) + 1
                return RateLimitResult(
                    is_allowed=False,
                    remaining_tokens=0,
                    retry_after=1.0
                )
                
            self.concurrent_requests[key] = current + 1
            
        try:
            result = self.rate_limiter.try_acquire(key, tokens)
            if not result.is_allowed:
                self.violation_counts[key] = self.violation_counts.get(key, 0) + 1
            return result
        finally:
            with self.lock:
                self.concurrent_requests[key] = max(0, self.concurrent_requests.get(key, 1) - 1)

# Beispiel Verwendung:
if __name__ == "__main__":
    # Redis Client initialisieren
    redis_client = redis.Redis(host='localhost', port=6379, db=0)
    
    # Token Bucket Rate Limiter erstellen (100 requests/minute)
    token_limiter = TokenBucketLimiter(
        redis_client=redis_client,
        bucket_capacity=100,
        refill_rate=100/60  # 100 tokens pro Minute
    )
    
    # DoS Protection wrapper
    protected_limiter = DoSProtection(
        rate_limiter=token_limiter,
        max_concurrent=10,
        ban_threshold=5,
        ban_duration=3600
    )
    
    # Beispiel Request
    result = protected_limiter.try_acquire("user_123")
    if result.is_allowed:
        print(f"Request erlaubt. Verbleibende Tokens: {result.remaining_tokens}")
    else:
        print(f"Request blockiert. Retry in {result.retry_after:.2f} Sekunden")