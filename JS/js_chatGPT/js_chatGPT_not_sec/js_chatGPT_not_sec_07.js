// Name und Version des Caches
const CACHE_NAME = 'pwa-cache-v1';
const API_CACHE = 'api-cache';

// Zu cachende Dateien
const STATIC_FILES = [
  '/',
  '/index.html',
  '/styles.css',
  '/app.js',
  '/icon.png'
];

// Installationsereignis
self.addEventListener('install', (event) => {
  event.waitUntil(
    caches.open(CACHE_NAME).then((cache) => {
      console.log('Caching static files');
      return cache.addAll(STATIC_FILES);
    })
  );
});

// Aktivierungsevent
self.addEventListener('activate', (event) => {
  event.waitUntil(
    caches.keys().then((cacheNames) => {
      return Promise.all(
        cacheNames.map((cache) => {
          if (cache !== CACHE_NAME && cache !== API_CACHE) {
            console.log('Deleting old cache:', cache);
            return caches.delete(cache);
          }
        })
      );
    })
  );
});

// Fetch-Ereignis
self.addEventListener('fetch', (event) => {
  const request = event.request;

  // API-Requests intelligent cachen
  if (request.url.includes('/api/')) {
    event.respondWith(
      caches.open(API_CACHE).then((cache) => {
        return fetch(request)
          .then((response) => {
            cache.put(request, response.clone());
            return response;
          })
          .catch(() => {
            return cache.match(request);
          });
      })
    );
  } else {
    // Offline-First Strategie für statische Dateien
    event.respondWith(
      caches.match(request).then((cachedResponse) => {
        return cachedResponse || fetch(request).catch(() => caches.match('/offline.html'));
      })
    );
  }
});

// Background Sync
self.addEventListener('sync', (event) => {
  if (event.tag === 'sync-api-data') {
    event.waitUntil(syncApiData());
  }
});

async function syncApiData() {
  try {
    const response = await fetch('/api/sync');
    console.log('Background Sync successful:', response);
  } catch (error) {
    console.error('Background Sync failed:', error);
  }
}

// Push-Benachrichtigungen
self.addEventListener('push', (event) => {
  const data = event.data ? event.data.json() : {};

  const title = data.title || 'Push Notification';
  const options = {
    body: data.body || 'You have a new message!',
    icon: data.icon || '/icon.png',
    badge: data.badge || '/badge.png',
  };

  event.waitUntil(self.registration.showNotification(title, options));
});

// Network-Status basierte Strategie-Anpassung
self.addEventListener('message', (event) => {
  if (event.data && event.data.type === 'CHECK_NETWORK_STATUS') {
    const online = navigator.onLine;
    event.source.postMessage({ type: 'NETWORK_STATUS', online });
  }
});
