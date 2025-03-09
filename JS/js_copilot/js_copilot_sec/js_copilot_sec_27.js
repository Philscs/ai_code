class WebCryptoWrapper {
    constructor() {
      this.crypto = window.crypto || window.msCrypto;
    }
  
    async generateKeyPair() {
      const keyPair = await this.crypto.subtle.generateKey(
        {
          name: "RSA-OAEP",
          modulusLength: 2048,
          publicExponent: new Uint8Array([0x01, 0x00, 0x01]),
          hash: { name: "SHA-256" },
        },
        true,
        ["encrypt", "decrypt"]
      );
  
      return keyPair;
    }
  
    async encrypt(data, publicKey) {
      const encodedData = new TextEncoder().encode(data);
      const encryptedData = await this.crypto.subtle.encrypt(
        {
          name: "RSA-OAEP",
        },
        publicKey,
        encodedData
      );
  
      return encryptedData;
    }
  
    async decrypt(encryptedData, privateKey) {
      const decryptedData = await this.crypto.subtle.decrypt(
        {
          name: "RSA-OAEP",
        },
        privateKey,
        encryptedData
      );
  
      const decodedData = new TextDecoder().decode(decryptedData);
      return decodedData;
    }
  
    async sign(data, privateKey) {
      const encodedData = new TextEncoder().encode(data);
      const signature = await this.crypto.subtle.sign(
        {
          name: "RSASSA-PKCS1-v1_5",
        },
        privateKey,
        encodedData
      );
  
      return signature;
    }
  
    async verify(data, signature, publicKey) {
      const encodedData = new TextEncoder().encode(data);
      const isVerified = await this.crypto.subtle.verify(
        {
          name: "RSASSA-PKCS1-v1_5",
        },
        publicKey,
        signature,
        encodedData
      );
  
      return isVerified;
    }
  
    generateSecureRandom(length) {
      const randomBytes = new Uint8Array(length);
      this.crypto.getRandomValues(randomBytes);
  
      return randomBytes;
    }
  
    async rotateKeyPair(oldKeyPair) {
      const newKeyPair = await this.generateKeyPair();
  
      // Perform key rotation logic here
  
      return newKeyPair;
    }
  }
  
  // Usage example:
  const cryptoWrapper = new WebCryptoWrapper();
  
  (async () => {
    const keyPair = await cryptoWrapper.generateKeyPair();
    console.log("Generated key pair:", keyPair);
  
    const data = "Hello, World!";
    const encryptedData = await cryptoWrapper.encrypt(data, keyPair.publicKey);
    console.log("Encrypted data:", encryptedData);
  
    const decryptedData = await cryptoWrapper.decrypt(encryptedData, keyPair.privateKey);
    console.log("Decrypted data:", decryptedData);
  
    const signature = await cryptoWrapper.sign(data, keyPair.privateKey);
    console.log("Signature:", signature);
  
    const isVerified = await cryptoWrapper.verify(data, signature, keyPair.publicKey);
    console.log("Is verified:", isVerified);
  
    const randomBytes = cryptoWrapper.generateSecureRandom(16);
    console.log("Random bytes:", randomBytes);
  
    const newKeyPair = await cryptoWrapper.rotateKeyPair(keyPair);
    console.log("New key pair:", newKeyPair);
  })();
  