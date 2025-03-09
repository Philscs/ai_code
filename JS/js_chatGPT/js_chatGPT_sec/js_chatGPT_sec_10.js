// Service Worker

const CACHE_NAME = 'pwa-cache-v1';
const STATIC_ASSETS = [
  '/',
  '/index.html',
  '/styles.css',
  '/script.js',
  '/favicon.ico'
];

self.addEventListener('install', (event) => {
  console.log('[Service Worker] Installing');
  event.waitUntil(
    caches.open(CACHE_NAME).then((cache) => {
      console.log('[Service Worker] Pre-caching static assets');
      return cache.addAll(STATIC_ASSETS);
    })
  );
});

self.addEventListener('activate', (event) => {
  console.log('[Service Worker] Activating');
  event.waitUntil(
    caches.keys().then((cacheNames) => {
      return Promise.all(
        cacheNames.map((cacheName) => {
          if (cacheName !== CACHE_NAME) {
            console.log('[Service Worker] Removing old cache:', cacheName);
            return caches.delete(cacheName);
          }
        })
      );
    })
  );
  return self.clients.claim();
});

self.addEventListener('fetch', (event) => {
  console.log('[Service Worker] Fetching:', event.request.url);
  event.respondWith(
    caches.match(event.request).then((cachedResponse) => {
      if (cachedResponse) {
        return cachedResponse;
      }
      return fetch(event.request).then((networkResponse) => {
        return caches.open(CACHE_NAME).then((cache) => {
          cache.put(event.request, networkResponse.clone());
          return networkResponse;
        });
      });
    })
  );
});

// Push Notifications
self.addEventListener('push', (event) => {
  console.log('[Service Worker] Push Received');
  const data = event.data ? event.data.json() : {};
  const title = data.title || 'Default Title';
  const options = {
    body: data.body || 'Default Body',
    icon: data.icon || '/favicon.ico',
    badge: data.badge || '/badge.png'
  };
  event.waitUntil(self.registration.showNotification(title, options));
});

// Background Sync
self.addEventListener('sync', (event) => {
  console.log('[Service Worker] Background Sync:', event.tag);
  if (event.tag === 'sync-posts') {
    event.waitUntil(syncPosts());
  }
});

async function syncPosts() {
  const posts = await getOfflinePosts();
  for (const post of posts) {
    await sendPostToServer(post);
    console.log('[Service Worker] Synced post:', post);
  }
}

async function getOfflinePosts() {
  // Retrieve offline posts from IndexedDB or other storage
  return [];
}

async function sendPostToServer(post) {
  // Implement sending post to the server
}

// Security Headers (implemented via HTTP server configuration)
// - Ensure the server adds Content-Security-Policy, X-Frame-Options, etc.
// - These cannot be added directly by the Service Worker.