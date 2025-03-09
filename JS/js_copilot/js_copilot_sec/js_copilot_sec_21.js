
// Content Script Management
function injectContentScript(scriptPath) {
  const script = document.createElement('script');
  script.src = chrome.extension.getURL(scriptPath);
  script.onload = function() {
    this.remove();
  };
  (document.head || document.documentElement).appendChild(script);
}

// Message Passing
chrome.runtime.onMessage.addListener(function(request, sender, sendResponse) {
  // Handle incoming messages from the extension or content scripts
  if (request.action === 'exampleAction') {
    // Do something with the message
    sendResponse({ result: 'Message received' });
  }
});

function sendMessageToBackgroundScript(message) {
  chrome.runtime.sendMessage(message, function(response) {
    // Handle response from the background script
    console.log(response);
  });
}

// Storage API
function saveDataToStorage(data) {
  chrome.storage.local.set(data, function() {
    console.log('Data saved to storage');
  });
}

function getDataFromStorage(keys) {
  chrome.storage.local.get(keys, function(result) {
    console.log(result);
  });
}

// CSP Compliance
const cspPolicy = {
  'default-src': ["'self'"],
  'script-src': ["'self'", "'unsafe-eval'", "'unsafe-inline'"],
  'style-src': ["'self'", "'unsafe-inline'"],
  'img-src': ["'self'", 'data:'],
  'connect-src': ["'self'"],
  'font-src': ["'self'"],
  'object-src': ["'none'"],
  'media-src': ["'self'"],
  'frame-src': ["'self'"],
  'worker-src': ["'self'"],
  'manifest-src': ["'self'"],
};

chrome.webRequest.onHeadersReceived.addListener(
  function(details) {
    const headers = details.responseHeaders;
    for (let i = headers.length - 1; i >= 0; --i) {
      const header = headers[i].name.toLowerCase();
      if (header === 'content-security-policy' || header === 'x-content-security-policy') {
        headers.splice(i, 1);
      }
    }
    headers.push({ name: 'Content-Security-Policy', value: generateCSPHeader() });
    return { responseHeaders: headers };
  },
  { urls: ['<all_urls>'], types: ['main_frame', 'sub_frame'] },
  ['blocking', 'responseHeaders']
);

function generateCSPHeader() {
  let cspHeader = '';
  for (const directive in cspPolicy) {
    cspHeader += `${directive} ${cspPolicy[directive].join(' ')}; `;
  }
  return cspHeader.trim();
}

// Permission System
function requestPermission(permission, callback) {
  chrome.permissions.request({ permissions: [permission] }, function(granted) {
    if (granted) {
      console.log('Permission granted');
      callback();
    } else {
      console.log('Permission denied');
    }
  });
}

function checkPermission(permission, callback) {
  chrome.permissions.contains({ permissions: [permission] }, function(result) {
    if (result) {
      console.log('Permission granted');
      callback();
    } else {
      console.log('Permission denied');
    }
  });
}
