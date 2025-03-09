// Feature Detection
function isScrollSupported() {
    return 'IntersectionObserver' in window && 'requestIdleCallback' in window;
  }
  
  function isModernBrowser() {
    return /Chrome|Safari|Firefox|Edge/i.test(navigator.userAgent);
  }
  
  function isHttps() {
    return window.location.protocol === 'https:';
  }
  
  function isDataLoaded() {
    return document.readyState === 'complete';
  }
  
  function isAccessible() {
    return document.querySelector('.accessible') !== null;
  }
  
  // Fallback-Strategie
  function fallbackStrategy() {
    if (!isModernBrowser()) {
      console.log('Lade die Fallback-Maske');
    }
    // Hier kannst du die Fallback-Maske laden und renderen
  }
  
  // Benutzeroberfläche load
  function loadInterface() {
    if (isScrollSupported()) {
      const observer = new IntersectionObserver((entries) => {
        if (entries[0].isIntersecting) {
          console.log('Lade die Oberfläche');
        }
      }, {
        rootMargin: '50px', // Höhe des Root-Elements
        threshold: 1.0, // Full Intersection
      });
  
      observer.observe(document.querySelector('.oberflache'));
    } else {
      console.log('Scroll-Detection nicht unterstützt');
    }
  }
  
  // sichere Verbindung
  function loadSecureConnection() {
    if (!isHttps()) {
      console.log('Lade die sichere Maske');
      // Hier kannst du die sichere Maske laden und renderen
    }
  }
  
  // Daten laden
  function load_data() {
    if (!isDataLoaded()) {
      console.log('Lade die Daten');
      // Hier kannst du die Daten laden und renderen
    }
  }
  
  // Zugänglichkeit überprüfen
  function checkAccessibility() {
    if (!isAccessible()) {
      console.log('Lade die Accessibillity-Maske');
      // Hier kannst du die Accessibillity-Maske laden und renderen
    }
  }
  
  // Alle Funktionen ausführen
  function init() {
    fallbackStrategy();
    loadInterface();
    loadSecureConnection();
    load_data();
    checkAccessibility();
  }
  
  init();