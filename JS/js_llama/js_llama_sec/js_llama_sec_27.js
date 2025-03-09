class WebCryptoAPI {
    constructor() {
      this.keyStore = new Map();
      this.crypto = window.crypto || window.msCrypto;
      if (!this.crypto) throw new Error("Web Crypto API nicht verfügbar");
    }
  
    createRSAKey(keySize) {
      const algorithm = "RSA-OAEP";
      const options = {
        keySize,
      };
      return new Promise((resolve, reject) => {
        this.crypto.subtle.generateKey({
          name: algorithm,
          modulusLength: options.keySize,
          publicExponent: new Uint8Array([1, 0, 1]),
        }, true, ["encrypt", "decrypt", "sign", "verify"])
        .then((keyPair) => {
          resolve(keyPair);
        })
        .catch((error) => {
          reject(error);
        });
      });
    }
  
    storeRSAKey(keyPair, label) {
      return new Promise((resolve, reject) => {
        this.keyStore.set(label, keyPair);
        resolve();
      });
    }
  
    retrieveRSAKey(label) {
      return new Promise((resolve, reject) => {
        if (this.keyStore.has(label)) {
          const key = this.keyStore.get(label);
          resolve(key);
        } else {
          reject(new Error(`Kein Schlüssel mit dem Label ${label} gefunden`));
        }
      });
    }
  
    rotateRSAKey(label) {
      return new Promise((resolve, reject) => {
        const storedKey = this.retrieveRSAKey(label).then((keyPair) => {
          if (!keyPair) {
            reject(new Error(`Kein Schlüssel mit dem Label ${label} gefunden`));
            return;
          }
          const newKeySize = keyPair.modulusLength * 2;
          return this.createRSAKey(newKeySize);
        }).then((newKeyPair) => {
          this.storeRSAKey(newKeyPair, label);
          resolve(newKeyPair);
        });
      });
    }
  
    sign(data, keyPair) {
      const algorithm = "RSA-PSS";
      const hash = new TextEncoder().encode("SHA-256");
      return new Promise((resolve, reject) => {
        this.crypto.subtle.sign(algorithm, keyPair.privateKey, data, hash)
          .then((signature) => {
            resolve(signature);
          })
          .catch((error) => {
            reject(error);
          });
      });
    }
  
    verify(signature, keyPair) {
      const algorithm = "RSA-PSS";
      const hash = new TextEncoder().encode("SHA-256");
      return new Promise((resolve, reject) => {
        this.crypto.subtle.verify(algorithm, keyPair.publicKey, signature, hash)
          .then((isValid) => {
            resolve(isValid);
          })
          .catch((error) => {
            reject(error);
          });
      });
    }
  
    secureRandom() {
      const algorithm = "SecureRandom";
      return new Promise((resolve, reject) => {
        this.crypto.subtle.generateKey({
          name: algorithm,
        }, false, ["generateKeyPair"])
        .then((keyPair) => {
          resolve(keyPair);
        })
        .catch((error) => {
          reject(error);
        });
      });
    }
  
    secureRandomBytes() {
      const algorithm = "SecureRandom";
      return new Promise((resolve, reject) => {
        this.crypto.subtle.generateKey({
          name: algorithm,
        }, false, ["generateKeyPair"])
        .then((keyPair) => {
          resolve(keyPair);
        })
        .catch((error) => {
          reject(error);
        });
      });
    }
  }
  
  // Beispiel für die Verwendung des Web-Crypto-API-Wrappers:
  if (typeof window !== 'undefined') {
    const webCrypto = new WebCryptoAPI();
  
    webCrypto.createRSAKey(2048).then((keyPair) => {
      console.log(keyPair);
  
      webCrypto.storeRSAKey(keyPair, "my-rsa-key").then(() => {
        console.log("Schlüssel gespeichert");
      });
  
      webCrypto.retrieveRSAKey("my-rsa-key").then((storedKey) => {
        console.log(storedKey);
      }).catch((error) => {
        console.error(error.message);
      });
  
      webCrypto.rotateRSAKey("my-rsa-key").then((newKeyPair) => {
        console.log(newKeyPair);
      }).catch((error) => {
        console.error(error.message);
      });
    }).catch((error) => {
      console.error(error.message);
    });
  }
  