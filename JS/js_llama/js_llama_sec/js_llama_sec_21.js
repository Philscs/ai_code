const { browserExtension } = require('browser-extension');

class ContentScriptManager {
  constructor(id) {
    this.id = id;
    this.scripts = [];
  }

  addScript(script) {
    this.scripts.push(script);
  }

  run() {
    return new Promise((resolve, reject) => {
      browserExtension.contentScriptRunner
        .install(this.id)
        .then(() => {
          browserExtension.contentScriptRunner
            .sendMessages(this.id, { type: 'run', data: [] })
            .then(resolve)
            .catch(reject);
        })
        .catch(reject);
    });
  }
}

class MessagePassingService {
  constructor(id) {
    this.id = id;
    this.messageQueue = [];
  }

  send(message) {
    return new Promise((resolve, reject) => {
      browserExtension.contentScriptRunner
        .sendMessages(this.id, { type: 'message', data: message })
        .then(resolve)
        .catch(reject);
    });
  }

  receive() {
    return new Promise((resolve, reject) => {
      browserExtension.contentScriptRunner
        .onMessage(this.id, (message) => {
          resolve(message.data);
        })
        .catch(reject);
    });
  }
}

class StorageService {
  constructor(id) {
    this.id = id;
    this.storage = {};
  }

  set(key, value) {
    return new Promise((resolve, reject) => {
      browserExtension.storage.set({ [key]: value }, (response) => {
        resolve(response);
      })
      .catch(reject);
    });
  }

  get(key) {
    return new Promise((resolve, reject) => {
      browserExtension.storage.get([key], (response) => {
        resolve(response[key]);
      })
      .catch(reject);
    });
  }
}

class CspService {
  constructor(id) {
    this.id = id;
    this.cspPolicy = '';
  }

  set(cspPolicy) {
    return new Promise((resolve, reject) => {
      browserExtension.contentScriptRunner
        .updateCSP(this.id, cspPolicy)
        .then(resolve)
        .catch(reject);
    });
  }
}

class PermissionService {
  constructor(id) {
    this.id = id;
    this.permissions = [];
  }

  request(permission) {
    return new Promise((resolve, reject) => {
      browserExtension.permissions
        .request([permission], (response) => {
          resolve(response);
        })
        .catch(reject);
    });
  }
}

const contentScriptManager = new ContentScriptManager('my-script-id');
const messagePassingService = new MessagePassingService('my-script-id');
const storageService = new StorageService('my-storage-id');
const cspService = new CspService('my-csp-id');
const permissionService = new PermissionService('my-permission-id');

// Install content script
contentScriptManager.addScript(() => {
  // Your content script code here
});

// Run content script
contentScriptManager.run().then(() => {
  console.log('Content script installed and run successfully!');
}).catch((error) => {
  console.error('Error running content script:', error);
});

// Send message to content script
messagePassingService.send({ type: 'hello', data: 'world' }).then(() => {
  console.log('Message sent to content script successfully!');
}).catch((error) => {
  console.error('Error sending message:', error);
});

// Get stored value
storageService.get('my-key').then((value) => {
  console.log('Stored value retrieved successfully:', value);
}).catch((error) => {
  console.error('Error retrieving stored value:', error);
});

// Update CSP policy
cspService.set('default-src \'self\'; script-src \'self\' https://example.com;');

// Request permission
permissionService.request('activeTab').then((response) => {
  console.log('Permission granted:', response);
}).catch((error) => {
  console.error('Error requesting permission:', error);
});
