class CryptoWrapper {
    constructor() {
        this.keys = {};
    }

    /**
     * Generate a cryptographic key pair
     * @param {string} keyName - Name for the key pair
     * @param {string} algorithm - Algorithm to use (e.g., "RSASSA-PKCS1-v1_5")
     * @param {object} options - Additional algorithm options
     */
    async generateKeyPair(keyName, algorithm, options) {
        const keyPair = await crypto.subtle.generateKey(
            {
                name: algorithm,
                ...options,
            },
            true, // extractable
            ["sign", "verify"]
        );
        this.keys[keyName] = keyPair;
    }

    /**
     * Encrypt data with a symmetric key
     * @param {CryptoKey} key - Symmetric key
     * @param {Uint8Array} data - Data to encrypt
     * @returns {Promise<ArrayBuffer>} - Encrypted data
     */
    async encrypt(key, data) {
        const iv = crypto.getRandomValues(new Uint8Array(12)); // GCM IV
        const encrypted = await crypto.subtle.encrypt(
            {
                name: "AES-GCM",
                iv,
            },
            key,
            data
        );
        return { iv, encrypted };
    }

    /**
     * Decrypt data with a symmetric key
     * @param {CryptoKey} key - Symmetric key
     * @param {ArrayBuffer} encrypted - Encrypted data
     * @param {Uint8Array} iv - Initialization vector used during encryption
     * @returns {Promise<ArrayBuffer>} - Decrypted data
     */
    async decrypt(key, encrypted, iv) {
        return crypto.subtle.decrypt(
            {
                name: "AES-GCM",
                iv,
            },
            key,
            encrypted
        );
    }

    /**
     * Sign data using a private key
     * @param {CryptoKey} privateKey - Private key
     * @param {Uint8Array} data - Data to sign
     * @returns {Promise<ArrayBuffer>} - Digital signature
     */
    async sign(privateKey, data) {
        return crypto.subtle.sign(
            {
                name: "RSASSA-PKCS1-v1_5",
            },
            privateKey,
            data
        );
    }

    /**
     * Verify a digital signature
     * @param {CryptoKey} publicKey - Public key
     * @param {ArrayBuffer} signature - Digital signature
     * @param {Uint8Array} data - Original data
     * @returns {Promise<boolean>} - Whether the signature is valid
     */
    async verify(publicKey, signature, data) {
        return crypto.subtle.verify(
            {
                name: "RSASSA-PKCS1-v1_5",
            },
            publicKey,
            signature,
            data
        );
    }

    /**
     * Generate a secure random key for AES encryption
     * @param {number} keyLength - Length of the key in bits (128, 192, 256)
     * @returns {Promise<CryptoKey>} - Generated key
     */
    async generateSymmetricKey(keyLength = 256) {
        return crypto.subtle.generateKey(
            {
                name: "AES-GCM",
                length: keyLength,
            },
            true,
            ["encrypt", "decrypt"]
        );
    }

    /**
     * Rotate an existing key pair
     * @param {string} keyName - Name of the key pair to rotate
     * @param {string} algorithm - Algorithm for the new key pair
     * @param {object} options - Additional algorithm options
     */
    async rotateKeyPair(keyName, algorithm, options) {
        await this.generateKeyPair(keyName, algorithm, options);
    }

    /**
     * Generate secure random data
     * @param {number} length - Length of random data to generate
     * @returns {Uint8Array} - Random bytes
     */
    generateRandomData(length) {
        return crypto.getRandomValues(new Uint8Array(length));
    }
}

// Usage example
(async () => {
    const cryptoWrapper = new CryptoWrapper();

    // Key management
    await cryptoWrapper.generateKeyPair("myKey", "RSASSA-PKCS1-v1_5", { modulusLength: 2048, publicExponent: new Uint8Array([1, 0, 1]) });
    const { publicKey, privateKey } = cryptoWrapper.keys["myKey"];

    // Encryption/Decryption
    const symmetricKey = await cryptoWrapper.generateSymmetricKey();
    const data = new TextEncoder().encode("Hello, secure world!");
    const { iv, encrypted } = await cryptoWrapper.encrypt(symmetricKey, data);
    const decrypted = await cryptoWrapper.decrypt(symmetricKey, encrypted, iv);
    console.log(new TextDecoder().decode(decrypted));

    // Digital Signatures
    const signature = await cryptoWrapper.sign(privateKey, data);
    const isValid = await cryptoWrapper.verify(publicKey, signature, data);
    console.log("Signature valid:", isValid);

    // Secure Random
    const randomBytes = cryptoWrapper.generateRandomData(16);
    console.log("Random bytes:", randomBytes);

    // Key Rotation
    await cryptoWrapper.rotateKeyPair("myKey", "RSASSA-PKCS1-v1_5", { modulusLength: 2048, publicExponent: new Uint8Array([1, 0, 1]) });
})();
