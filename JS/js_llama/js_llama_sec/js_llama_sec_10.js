// service-worker.js

importScripts('https://cdn.jsdelivr.net/npm/webpush-serve@1.5.2/dist/webpush.min.js');
importScripts('https://cdn.jsdelivr.net/npm/caching-strategy@1.3.0/dist/index.min.js');

const CACHE_NAME = 'pwa-cache';
const FALLBACK_URL = '/fallback.html';

self.addEventListener('install', (event) => {
  event.waitUntil(
    caches.open(CACHE_NAME).then((cache) => {
      return cache.addAll([
        '/',
        'index.html',
        'styles.css',
        'script.js',
        'manifest.json',
      ]);
    })
  );
});

self.addEventListener('fetch', (event) => {
  event.respondWith(
    caches.match(event.request).then((response) => {
      if (response) return response;
      return fetch(event.request);
    }).catch(() => {
      return caches.match(FALLBACK_URL);
    })
  );
});

self.addEventListener('push', (event) => {
  const title = 'Push Notification';
  const message = 'Hello, World!';
  event.waitUntil(
    self.registration.showNotification(title, { icon: '/icon.png' });
  );
});

self.addEventListener('sync', (event) => {
  // Background sync implementation
  console.log('Background Sync');
});

self.addEventListener('activate', (event) => {
  event.waitUntil(self.caches.keys().then((cacheNames) => {
    return cacheNames.forEach((cacheName) => {
      if (cacheName !== CACHE_NAME && self.caches.match(cacheName)) {
        self.caches.delete(cacheName);
      }
    });
  }));
});

self.addEventListener('skipWaiting', () => {
  console.log('Service Worker aktiv');
});

self.addEventListener('push', (event) => {
  const title = 'Push Notification';
  const message = 'Hello, World!';
  event.waitUntil(
    self.registration.showNotification(title, { icon: '/icon.png' });
  );
});
self.addEventListener('sync', (event) => {
  event.waitUntil(
    fetch('https://api.example.com/sync')
      .then((response) => response.json())
      .catch((error) => console.error(error))
  );
});

import { cachingStrategy } from 'caching-strategy';

self.addEventListener('fetch', (event) => {
  event.respondWith(
    cachingStrategy.getCache(CACHE_NAME).then((cache) => {
      return cache.fetch(event.request);
    }).catch(() => {
      return fetch(event.request);
    })
  );
});