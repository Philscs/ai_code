// Content Script Manager
class ContentScriptManager {
    constructor() {
        this.scripts = new Map();
    }

    // Register a new content script
    register(scriptId, config) {
        if (this.scripts.has(scriptId)) {
            throw new Error(`Script ${scriptId} already registered`);
        }

        const scriptConfig = {
            matches: config.matches || [],
            js: config.js || [],
            css: config.css || [],
            runAt: config.runAt || 'document_idle',
            allFrames: config.allFrames || false
        };

        this.scripts.set(scriptId, scriptConfig);
    }

    // Inject script into a specific tab
    async injectScript(tabId, scriptId) {
        const config = this.scripts.get(scriptId);
        if (!config) {
            throw new Error(`Script ${scriptId} not found`);
        }

        try {
            // Inject CSS files
            for (const cssFile of config.css) {
                await chrome.scripting.insertCSS({
                    target: { tabId },
                    files: [cssFile]
                });
            }

            // Inject JavaScript files
            for (const jsFile of config.js) {
                await chrome.scripting.executeScript({
                    target: { tabId },
                    files: [jsFile],
                    runAt: config.runAt
                });
            }
        } catch (error) {
            console.error(`Failed to inject script ${scriptId}:`, error);
            throw error;
        }
    }
}

// Message Passing System
class MessageBus {
    constructor() {
        this.handlers = new Map();
        this.setupListeners();
    }

    setupListeners() {
        // Listen for messages from content scripts
        chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
            this.handleMessage(message, sender, sendResponse);
            return true; // Keep the message channel open for async responses
        });
    }

    // Register a message handler
    on(messageType, handler) {
        if (!this.handlers.has(messageType)) {
            this.handlers.set(messageType, new Set());
        }
        this.handlers.get(messageType).add(handler);
    }

    // Remove a message handler
    off(messageType, handler) {
        const handlers = this.handlers.get(messageType);
        if (handlers) {
            handlers.delete(handler);
        }
    }

    // Send a message to a specific tab
    async sendToTab(tabId, messageType, data) {
        try {
            return await chrome.tabs.sendMessage(tabId, {
                type: messageType,
                payload: data
            });
        } catch (error) {
            console.error(`Failed to send message to tab ${tabId}:`, error);
            throw error;
        }
    }

    // Send a message to the background script
    async sendToBackground(messageType, data) {
        try {
            return await chrome.runtime.sendMessage({
                type: messageType,
                payload: data
            });
        } catch (error) {
            console.error('Failed to send message to background:', error);
            throw error;
        }
    }

    // Handle incoming messages
    async handleMessage(message, sender, sendResponse) {
        const { type, payload } = message;
        const handlers = this.handlers.get(type);

        if (!handlers) {
            console.warn(`No handlers registered for message type: ${type}`);
            return;
        }

        try {
            for (const handler of handlers) {
                await handler(payload, sender);
            }
        } catch (error) {
            console.error(`Error handling message type ${type}:`, error);
            throw error;
        }
    }
}

// Storage System
class StorageManager {
    constructor(storageArea = 'sync') {
        this.storage = chrome.storage[storageArea];
    }

    // Get data from storage
    async get(key) {
        try {
            const result = await this.storage.get(key);
            return key ? result[key] : result;
        } catch (error) {
            console.error('Failed to get data from storage:', error);
            throw error;
        }
    }

    // Set data in storage
    async set(key, value) {
        try {
            await this.storage.set({ [key]: value });
        } catch (error) {
            console.error('Failed to set data in storage:', error);
            throw error;
        }
    }

    // Remove data from storage
    async remove(key) {
        try {
            await this.storage.remove(key);
        } catch (error) {
            console.error('Failed to remove data from storage:', error);
            throw error;
        }
    }

    // Clear all data from storage
    async clear() {
        try {
            await this.storage.clear();
        } catch (error) {
            console.error('Failed to clear storage:', error);
            throw error;
        }
    }

    // Listen for storage changes
    onChanged(callback) {
        chrome.storage.onChanged.addListener((changes, areaName) => {
            callback(changes, areaName);
        });
    }
}

// Permission Manager
class PermissionManager {
    constructor() {
        this.requiredPermissions = new Set();
        this.optionalPermissions = new Set();
    }

    // Register required permissions
    registerRequired(permissions) {
        permissions.forEach(permission => this.requiredPermissions.add(permission));
    }

    // Register optional permissions
    registerOptional(permissions) {
        permissions.forEach(permission => this.optionalPermissions.add(permission));
    }

    // Check if a permission is granted
    async hasPermission(permission) {
        try {
            const result = await chrome.permissions.contains({
                permissions: [permission]
            });
            return result;
        } catch (error) {
            console.error(`Failed to check permission ${permission}:`, error);
            throw error;
        }
    }

    // Request optional permissions
    async requestPermission(permission) {
        if (!this.optionalPermissions.has(permission)) {
            throw new Error(`Permission ${permission} not registered as optional`);
        }

        try {
            const granted = await chrome.permissions.request({
                permissions: [permission]
            });
            return granted;
        } catch (error) {
            console.error(`Failed to request permission ${permission}:`, error);
            throw error;
        }
    }

    // Remove optional permissions
    async removePermission(permission) {
        if (!this.optionalPermissions.has(permission)) {
            throw new Error(`Permission ${permission} not registered as optional`);
        }

        try {
            const removed = await chrome.permissions.remove({
                permissions: [permission]
            });
            return removed;
        } catch (error) {
            console.error(`Failed to remove permission ${permission}:`, error);
            throw error;
        }
    }
}

// CSP Compliance Helper
class CSPManager {
    constructor() {
        this.policies = new Map();
    }

    // Add a CSP directive
    addDirective(directive, value) {
        if (!this.policies.has(directive)) {
            this.policies.set(directive, new Set());
        }
        value.split(' ').forEach(v => this.policies.get(directive).add(v));
    }

    // Remove a CSP directive
    removeDirective(directive, value) {
        const directiveSet = this.policies.get(directive);
        if (directiveSet) {
            value.split(' ').forEach(v => directiveSet.delete(v));
        }
    }

    // Generate CSP header string
    generateCSPHeader() {
        const parts = [];
        for (const [directive, values] of this.policies) {
            if (values.size > 0) {
                parts.push(`${directive} ${[...values].join(' ')}`);
            }
        }
        return parts.join('; ');
    }

    // Apply CSP to the current document
    applyToDocument() {
        const meta = document.createElement('meta');
        meta.setAttribute('http-equiv', 'Content-Security-Policy');
        meta.setAttribute('content', this.generateCSPHeader());
        document.head.appendChild(meta);
    }
}

// Main Extension Framework
class ExtensionFramework {
    constructor() {
        this.contentScriptManager = new ContentScriptManager();
        this.messageBus = new MessageBus();
        this.storageManager = new StorageManager();
        this.permissionManager = new PermissionManager();
        this.cspManager = new CSPManager();
    }

    // Initialize the framework
    async initialize() {
        // Set up default CSP directives
        this.cspManager.addDirective('default-src', "'self'");
        this.cspManager.addDirective('script-src', "'self'");
        this.cspManager.addDirective('style-src', "'self'");

        // Set up message handling
        this.messageBus.on('storage:get', async (data, sender) => {
            const result = await this.storageManager.get(data.key);
            return result;
        });

        this.messageBus.on('storage:set', async (data, sender) => {
            await this.storageManager.set(data.key, data.value);
        });
    }
}

// Export the framework
export default ExtensionFramework;