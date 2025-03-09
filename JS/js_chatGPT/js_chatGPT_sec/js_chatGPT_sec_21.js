// Basic Browser Extension Framework

// =========================
// Content Script Management
// =========================
const ContentScriptManager = {
    injectScript: (tabId, scriptFile) => {
      return new Promise((resolve, reject) => {
        chrome.scripting.executeScript({
          target: { tabId },
          files: [scriptFile],
        }, () => {
          if (chrome.runtime.lastError) {
            reject(chrome.runtime.lastError);
          } else {
            resolve();
          }
        });
      });
    },
  };
  
  // =========================
  // Message Passing
  // =========================
  const MessageHandler = {
    sendMessage: (message, callback) => {
      chrome.runtime.sendMessage(message, callback);
    },
    addListener: (callback) => {
      chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
        callback(message, sender, sendResponse);
        return true; // Keep the messaging channel open for async responses
      });
    },
  };
  
  // =========================
  // Storage API
  // =========================
  const StorageAPI = {
    get: (key) => {
      return new Promise((resolve, reject) => {
        chrome.storage.local.get(key, (result) => {
          if (chrome.runtime.lastError) {
            reject(chrome.runtime.lastError);
          } else {
            resolve(result[key]);
          }
        });
      });
    },
    set: (key, value) => {
      return new Promise((resolve, reject) => {
        chrome.storage.local.set({ [key]: value }, () => {
          if (chrome.runtime.lastError) {
            reject(chrome.runtime.lastError);
          } else {
            resolve();
          }
        });
      });
    },
    remove: (key) => {
      return new Promise((resolve, reject) => {
        chrome.storage.local.remove(key, () => {
          if (chrome.runtime.lastError) {
            reject(chrome.runtime.lastError);
          } else {
            resolve();
          }
        });
      });
    },
  };
  
  // =========================
  // CSP Compliance
  // =========================
  const CSPManager = {
    setPolicy: (policy) => {
      // Placeholder for applying CSP policies dynamically.
      console.log("CSP Policy applied: ", policy);
    },
  };
  
  // =========================
  // Permission System
  // =========================
  const PermissionSystem = {
    request: (permissions) => {
      return new Promise((resolve, reject) => {
        chrome.permissions.request({ permissions }, (granted) => {
          if (granted) {
            resolve();
          } else {
            reject(new Error("Permission denied"));
          }
        });
      });
    },
    check: (permissions) => {
      return new Promise((resolve) => {
        chrome.permissions.contains({ permissions }, (result) => {
          resolve(result);
        });
      });
    },
  };
  
  // =========================
  // Framework Initialization
  // =========================
  const BrowserExtensionFramework = {
    ContentScriptManager,
    MessageHandler,
    StorageAPI,
    CSPManager,
    PermissionSystem,
  };
  
  export default BrowserExtensionFramework;
  
