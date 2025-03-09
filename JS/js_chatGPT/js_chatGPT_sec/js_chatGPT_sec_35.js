// Hauptskript (Service Worker, Content Script und Popup-Logik)

// Hintergrundlogik (Service Worker)
chrome.runtime.onMessage.addListener(async (message, sender, sendResponse) => {
    if (message.action === 'fetchData') {
      const csrfToken = await getCsrfToken(); // Hole ein sicheres CSRF-Token
      try {
        const response = await fetch('https://example.com/api/data', {
          method: 'GET',
          headers: {
            'Authorization': `Bearer ${csrfToken}`
          }
        });
  
        const data = await response.json();
        sendResponse({ success: true, data });
      } catch (error) {
        sendResponse({ success: false, error: error.message });
      }
    }
    return true; // Asynchrone Antwort zulassen
  });
  
  async function getCsrfToken() {
    // Simuliertes Token - Hole dieses aus einer API
    return 'secureRandomToken123';
  }
  
  // Content Script
  if (typeof chrome.runtime !== 'undefined') {
    chrome.runtime.sendMessage({ action: 'fetchData' }, (response) => {
      if (response.success) {
        console.log('Daten:', response.data);
      } else {
        console.error('Fehler beim Abrufen der Daten:', response.error);
      }
    });
  }
  
  // Popup-Skript
  document.addEventListener('DOMContentLoaded', () => {
    const contentDiv = document.getElementById('content');
    const fetchButton = document.getElementById('fetchData');
  
    // Button-Klick-Event
    fetchButton.addEventListener('click', () => {
      chrome.runtime.sendMessage({ action: 'fetchData' }, (response) => {
        if (response.success) {
          contentDiv.textContent = JSON.stringify(response.data, null, 2);
        } else {
          contentDiv.textContent = `Fehler: ${response.error}`;
        }
      });
    });
  });
  