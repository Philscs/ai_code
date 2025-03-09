// BEGIN: Token Bucket Algorithm
class TokenBucket {
  constructor(capacity, fillRate) {
    this.capacity = capacity;
    this.tokens = capacity;
    this.fillRate = fillRate;
    this.lastRefillTime = Date.now();
  }

  refill() {
    const now = Date.now();
    const elapsedTime = now - this.lastRefillTime;
    const tokensToAdd = elapsedTime * (this.fillRate / 1000);
    this.tokens = Math.min(this.tokens + tokensToAdd, this.capacity);
    this.lastRefillTime = now;
  }

  consume() {
    if (this.tokens >= 1) {
      this.tokens--;
      return true;
    }
    return false;
  }
}
// END: Token Bucket Algorithm

// BEGIN: IP-based Limits
const ipLimits = new Map();

function checkIPLimit(ip) {
  if (!ipLimits.has(ip)) {
    ipLimits.set(ip, new TokenBucket(100, 10)); // Example: 100 requests per 10 seconds
  }

  const bucket = ipLimits.get(ip);
  bucket.refill();

  return bucket.consume();
}
// END: IP-based Limits

// BEGIN: User-based Limits
const userLimits = new Map();

function checkUserLimit(userId) {
  if (!userLimits.has(userId)) {
    userLimits.set(userId, new TokenBucket(1000, 100)); // Example: 1000 requests per 100 seconds
  }

  const bucket = userLimits.get(userId);
  bucket.refill();

  return bucket.consume();
}
// END: User-based Limits

// BEGIN: Distributed Rate Limiting
function checkRateLimit(ip, userId) {
  const ipAllowed = checkIPLimit(ip);
  const userAllowed = checkUserLimit(userId);

  return ipAllowed && userAllowed;
}
// END: Distributed Rate Limiting

// BEGIN: Graceful Degradation
function handleRequest(ip, userId) {
  if (checkRateLimit(ip, userId)) {
    // Process the request
    console.log("Request processed successfully");
  } else {
    // Handle rate limit exceeded
    console.log("Rate limit exceeded");
  }
}
// END: Graceful Degradation
