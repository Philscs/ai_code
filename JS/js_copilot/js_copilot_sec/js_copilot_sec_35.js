    class ExtensionSecurity {
        constructor() {
            this.VERSION = '1.0.0';
            this.CSP_HEADER = "default-src 'self'; script-src 'self'; object-src 'none';";
            this.trustedDomains = ['trusted-domain.com'];
        }

        // Content Security Policy Implementation
        setupCSP() {
            const meta = document.createElement('meta');
            meta.httpEquiv = 'Content-Security-Policy';
            meta.content = this.CSP_HEADER;
            document.head.appendChild(meta);
        }

        // XSS Prevention
        sanitizeInput(input) {
            if (!input) return '';

            return input
                .replace(/&/g, '&amp;')
                .replace(/</g, '&lt;')
                .replace(/>/g, '&gt;')
                .replace(/"/g, '&quot;')
                .replace(/'/g, '&#x27;')
                .replace(/\//g, '&#x2F;');
        }

        validateURL(url) {
            try {
                const parsedUrl = new URL(url);
                return this.trustedDomains.some(domain => parsedUrl.hostname.endsWith(domain));
            } catch (e) {
                return false;
            }
        }

        // CSRF Protection
        generateCSRFToken() {
            const array = new Uint8Array(32);
            crypto.getRandomValues(array);
            return Array.from(array, byte => byte.toString(16).padStart(2, '0')).join('');
        }

        async setCSRFToken() {
            const token = this.generateCSRFToken();
            await chrome.storage.local.set({ csrfToken: token });
            return token;
        }

        async validateCSRFToken(token) {
            const { csrfToken } = await chrome.storage.local.get('csrfToken');
            return token === csrfToken;
        }

        // Privilege Separation
        async checkPermission(permission) {
            try {
                const result = await chrome.permissions.contains({
                    permissions: [permission]
                });
                return result;
            } catch (error) {
                console.error('Permission check failed:', error);
                return false;
            }
        }

        // Update Mechanism
        async checkForUpdates() {
            try {
                const response = await fetch('https://trusted-domain.com/extension/version.json', {
                    headers: {
                        'X-CSRF-Token': await this.getStoredCSRFToken()
                    }
                });

                if (!response.ok) throw new Error('Update check failed');

                const { version, updateUrl } = await response.json();

                if (this.compareVersions(version, this.VERSION) > 0) {
                    return {
                        needsUpdate: true,
                        newVersion: version,
                        updateUrl
                    };
                }

                return { needsUpdate: false };
            } catch (error) {
                console.error('Update check failed:', error);
                throw error;
            }
        }

        compareVersions(v1, v2) {
            const parts1 = v1.split('.').map(Number);
            const parts2 = v2.split('.').map(Number);

            for (let i = 0; i < Math.max(parts1.length, parts2.length); i++) {
                const part1 = parts1[i] || 0;
                const part2 = parts2[i] || 0;

                if (part1 > part2) return 1;
                if (part1 < part2) return -1;
            }

            return 0;
        }

        // Secure Storage
        async secureSet(key, value) {
            const encryptedValue = await this.encrypt(JSON.stringify(value));
            await chrome.storage.local.set({ [key]: encryptedValue });
        }

        async secureGet(key) {
            const { [key]: encryptedValue } = await chrome.storage.local.get(key);
            if (!encryptedValue) return null;

            const decryptedValue = await this.decrypt(encryptedValue);
            return JSON.parse(decryptedValue);
        }

        // Encryption helpers
        async encrypt(data) {
            const encoder = new TextEncoder();
            const key = await this.getEncryptionKey();
            const iv = crypto.getRandomValues(new Uint8Array(12));

            const encryptedData = await crypto.subtle.encrypt(
                { name: 'AES-GCM', iv },
                key,
                encoder.encode(data)
            );

            return {
                iv: Array.from(iv),
                data: Array.from(new Uint8Array(encryptedData))
            };
        }

        async decrypt(encryptedData) {
            const decoder = new TextDecoder();
            const key = await this.getEncryptionKey();

            const decryptedData = await crypto.subtle.decrypt(
                { name: 'AES-GCM', iv: new Uint8Array(encryptedData.iv) },
                key,
                new Uint8Array(encryptedData.data)
            );

            return decoder.decode(decryptedData);
        }

        async getEncryptionKey() {
            let key = await chrome.storage.local.get('encryptionKey');

            if (!key.encryptionKey) {
                key = await crypto.subtle.generateKey(
                    { name: 'AES-GCM', length: 256 },
                    true,
                    ['encrypt', 'decrypt']
                );

                await chrome.storage.local.set({ encryptionKey: key });
            }

            return key.encryptionKey;
        }
    }

    // Usage example
    const security = new ExtensionSecurity();

    // Initialize security measures
    document.addEventListener('DOMContentLoaded', async () => {
        // Setup CSP
        security.setupCSP();

        // Generate initial CSRF token
        await security.setCSRFToken();

        // Check for updates
        try {
            const updateStatus = await security.checkForUpdates();
            if (updateStatus.needsUpdate) {
                console.log(`New version ${updateStatus.newVersion} available`);
            }
        } catch (error) {
            console.error('Update check failed:', error);
        }
    });

    // Example of secure data handling
    async function handleUserInput(userInput) {
        // Sanitize input
        const sanitizedInput = security.sanitizeInput(userInput);

        // Store securely
        await security.secureSet('userInput', sanitizedInput);

        // Validate URL if present
        if (userInput.includes('http')) {
            const isValid = security.validateURL(userInput);
            if (!isValid) {
                throw new Error('Invalid URL detected');
            }
        }
    }

    // Example of secure API call
    async function makeSecureAPICall(endpoint) {
        if (!security.validateURL(endpoint)) {
            throw new Error('Invalid endpoint URL');
        }

        const csrfToken = await security.generateCSRFToken();

        const response = await fetch(endpoint, {
            headers: {
                'X-CSRF-Token': csrfToken
            }
        });

        return response.json();
    }
