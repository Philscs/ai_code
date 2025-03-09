// service-worker.js

class ServiceWorker {
    constructor() {
      this.cacheName = 'api-cache';
      this.networkStatus = null;
    }
  
    install(event) {
      event.waitUntil(
        caches.open(this.cacheName).then((cache) => {
          return cache.addAll([
            '/index.html',
            '/styles.css',
            '/script.js'
          ]);
        })
      );
    }
  
    fetch(event) {
      this.networkStatus = event.type === 'load';
  
      if (this.networkStatus && this.isNetworkReady()) {
        return fetch('/api/data').then((response) => response.json());
      } else if (!this.networkStatus && this.isOfflineReady()) {
        return caches.match(this.cacheName).then((cacheResponse) => cacheResponse.json());
      }
  
      return Promise.reject('Fehler beim Abrufen von Daten');
    }
  
    isNetworkReady() {
      return navigator.onLine;
    }
  
    isOfflineReady() {
      return !navigator.onLine && caches.keys().length > 0;
    }
  
    async syncEvent(event) {
      if (!this.syncEnabled()) {
        return;
      }
  
      try {
        const response = await fetch('/api/data');
        this.cacheData(response);
      } catch (error) {
        console.error('Fehler beim Synchronisieren:', error);
      }
    }
  
    async pushEvent(event) {
      if (!this.pushEnabled()) {
        return;
      }
  
      try {
        const notification = await navigator.serviceWorker.getNotification();
        if (notification) {
          this.sendPushNotification(notification.data);
        } else {
          console.log('Keine Push-Notification');
        }
      } catch (error) {
        console.error('Fehler beim Auslösen der Push-Notification:', error);
      }
    }
  
    syncEnabled() {
      return navigator.serviceWorker.getRegistration().then((registration) => 
  registration.pushOptions && registration.pushOptions.sync);
    }
  
    pushEnabled() {
      return navigator.serviceWorker.getRegistration().then((registration) => 
  registration.pushOptions && registration.pushOptions.body);
    }
  
    cacheData(response) {
      caches.open(this.cacheName).then((cache) => {
        cache.put('/api/data', response);
      });
    }
  
    sendPushNotification(data) {
      navigator.serviceWorker.getNotification().then((notification) => 
  notification.sendData(data));
    }
  }
  
  new ServiceWorker();
  

  // index.js
  
  class Index {
    constructor() {
      this.serviceWorkerScript = document.createElement('script');
      this.serviceWorkerScript.src = 'service-worker.js';
      this.serviceWorkerScript.async = true;
  
      navigator.serviceWorker.getRegistration().then((registration) => 
  registration.pushOptions.set({ userVisibleOnly: false, applicationServerKey: 
  'YOUR_APPLICATION_SERVER_KEY' }));
  
      this.registration = navigator.serviceWorker.getRegistration();
      if (this.registration) {
        this.registration.addEventListener('update', () => this.registration.postMessage({ action: 
  'install', data: 'Update' }));
        this.registration.addEventListener('activate', () => console.log('Service Worker aktiviert'));
        this.registration.addEventListener('fetch', (event) => console.log('Benutzer hat gefragt:', 
  event.request.url));
      }
    }
  
    run() {
      document.body.appendChild(this.serviceWorkerScript);
  
      navigator.serviceWorker.getRegistration().then((registration) => 
  registration.pushManager.subscribe({ userVisibleOnly: false }));
  
      fetch('/api/data')
        .then(response => response.json())
        .then(data => console.log('Daten:', data))
        .catch(error => console.error('Fehler beim Ausführen der API-Anfrage:', error));
    }
  }
  
  new Index().run();
  