// Cache Namen definieren
const CACHE_VERSION = 'v1';
const STATIC_CACHE = `static-${CACHE_VERSION}`;
const DYNAMIC_CACHE = `dynamic-${CACHE_VERSION}`;
const API_CACHE = `api-${CACHE_VERSION}`;

// Assets die immer gecacht werden sollen
const STATIC_ASSETS = [
  '/',
  '/index.html',
  '/styles/main.css',
  '/scripts/app.js',
  '/images/offline.svg'
];

// Installation des Service Workers
self.addEventListener('install', (event) => {
  event.waitUntil(
    caches.open(STATIC_CACHE)
      .then(cache => cache.addAll(STATIC_ASSETS))
      .then(() => self.skipWaiting())
  );
});

// Aktivierung und Cache-Cleanup
self.addEventListener('activate', (event) => {
  event.waitUntil(
    Promise.all([
      self.clients.claim(),
      // Alte Cache Versionen löschen
      caches.keys().then(keys => {
        return Promise.all(
          keys.filter(key => {
            return !key.includes(CACHE_VERSION);
          }).map(key => caches.delete(key))
        );
      })
    ])
  );
});

// Netzwerkstatus überwachen
let isOnline = true;
self.addEventListener('online', () => {
  isOnline = true;
  syncPendingRequests();
});
self.addEventListener('offline', () => {
  isOnline = false;
});

// Fetch Event Handler mit verschiedenen Strategien
self.addEventListener('fetch', (event) => {
  const request = event.request;
  const url = new URL(request.url);

  // API Calls
  if (url.pathname.startsWith('/api/')) {
    event.respondWith(handleApiRequest(request));
  }
  // Statische Assets
  else if (STATIC_ASSETS.includes(url.pathname)) {
    event.respondWith(handleStaticRequest(request));
  }
  // Dynamische Inhalte
  else {
    event.respondWith(handleDynamicRequest(request));
  }
});

// API Request Handler mit intelligentem Caching
async function handleApiRequest(request) {
  // Netzwerk-First Strategie für POST Requests
  if (request.method === 'POST') {
    try {
      const response = await fetch(request.clone());
      return response;
    } catch (error) {
      // Bei Offline-Status: Request in IndexedDB speichern
      await saveRequestForSync(request);
      return new Response(JSON.stringify({ 
        status: 'offline',
        message: 'Request wird synchronisiert sobald online' 
      }), {
        headers: { 'Content-Type': 'application/json' }
      });
    }
  }

  // Cache-dann-Netzwerk Strategie für GET Requests
  try {
    // Parallel aus Cache und Netzwerk laden
    const cachedResponse = await caches.match(request);
    const networkPromise = fetch(request.clone()).then(async response => {
      if (response.ok) {
        const cache = await caches.open(API_CACHE);
        await cache.put(request, response.clone());
      }
      return response;
    });

    return cachedResponse || networkPromise;
  } catch (error) {
    // Bei Netzwerkfehler: Cached Version zurückgeben
    const cachedResponse = await caches.match(request);
    if (cachedResponse) {
      return cachedResponse;
    }
    throw error;
  }
}

// Statische Assets Handler
async function handleStaticRequest(request) {
  const cache = await caches.open(STATIC_CACHE);
  const cachedResponse = await cache.match(request);
  
  if (cachedResponse) {
    // Im Hintergrund aktualisieren
    if (isOnline) {
      fetch(request).then(response => {
        if (response.ok) {
          cache.put(request, response);
        }
      });
    }
    return cachedResponse;
  }

  return fetch(request);
}

// Dynamische Inhalte Handler
async function handleDynamicRequest(request) {
  try {
    const response = await fetch(request);
    if (response.ok) {
      const cache = await caches.open(DYNAMIC_CACHE);
      await cache.put(request, response.clone());
    }
    return response;
  } catch (error) {
    const cachedResponse = await caches.match(request);
    if (cachedResponse) {
      return cachedResponse;
    }
    // Offline Fallback
    return caches.match('/offline.html');
  }
}

// Background Sync
let dbPromise;

function openDatabase() {
  if (!dbPromise) {
    dbPromise = new Promise((resolve, reject) => {
      const request = indexedDB.open('OfflineRequests', 1);
      
      request.onerror = () => reject(request.error);
      request.onsuccess = () => resolve(request.result);
      
      request.onupgradeneeded = (event) => {
        const db = event.target.result;
        if (!db.objectStoreNames.contains('requests')) {
          db.createObjectStore('requests', { keyPath: 'id', autoIncrement: true });
        }
      };
    });
  }
  return dbPromise;
}

async function saveRequestForSync(request) {
  const db = await openDatabase();
  const tx = db.transaction('requests', 'readwrite');
  const store = tx.objectStore('requests');
  
  const serializedRequest = {
    url: request.url,
    method: request.method,
    headers: Array.from(request.headers.entries()),
    body: await request.clone().text()
  };
  
  await store.add(serializedRequest);
}

async function syncPendingRequests() {
  if (!isOnline) return;

  const db = await openDatabase();
  const tx = db.transaction('requests', 'readwrite');
  const store = tx.objectStore('requests');
  
  const requests = await store.getAll();
  
  for (const request of requests) {
    try {
      await fetch(new Request(request.url, {
        method: request.method,
        headers: new Headers(request.headers),
        body: request.body
      }));
      
      await store.delete(request.id);
    } catch (error) {
      console.error('Sync failed:', error);
    }
  }
}

// Push Notifications
self.addEventListener('push', (event) => {
  if (!event.data) return;

  const data = event.data.json();
  const options = {
    body: data.body,
    icon: data.icon,
    badge: data.badge,
    data: data.data,
    actions: data.actions || [],
    vibrate: [100, 50, 100]
  };

  event.waitUntil(
    self.registration.showNotification(data.title, options)
  );
});

// Notification Click Handler
self.addEventListener('notificationclick', (event) => {
  event.notification.close();

  if (event.action) {
    // Handle notification action clicks
    handleNotificationAction(event.action, event.notification.data);
  } else {
    // Default click behavior
    event.waitUntil(
      clients.matchAll({ type: 'window' }).then(windowClients => {
        if (windowClients.length > 0) {
          windowClients[0].focus();
        } else {
          clients.openWindow('/');
        }
      })
    );
  }
});

// Periodische Sync (falls unterstützt)
self.addEventListener('periodicsync', (event) => {
  if (event.tag === 'content-sync') {
    event.waitUntil(syncContent());
  }
});

async function syncContent() {
  // Hier können regelmäßige Updates durchgeführt werden
  const cache = await caches.open(DYNAMIC_CACHE);
  // z.B. wichtige API-Endpunkte aktualisieren
  await updateImportantEndpoints(cache);
}

// Utility Funktionen
async function updateImportantEndpoints(cache) {
  const endpoints = [
    '/api/important-data',
    '/api/user-settings'
  ];

  for (const endpoint of endpoints) {
    try {
      const response = await fetch(endpoint);
      if (response.ok) {
        await cache.put(endpoint, response);
      }
    } catch (error) {
      console.error(`Failed to update ${endpoint}:`, error);
    }
  }
}

function handleNotificationAction(action, data) {
  switch (action) {
    case 'reply':
      // Handle reply action
      break;
    case 'archive':
      // Handle archive action
      break;
    default:
      console.log('Unknown action:', action);
  }
}