class CryptoWrapper {
    constructor() {
      if (!window.crypto || !window.crypto.subtle) {
        throw new Error('Web Crypto API wird nicht unterstützt');
      }
      this.crypto = window.crypto;
      this.subtle = window.crypto.subtle;
      
      // Standardalgorithmen
      this.algorithms = {
        aes: {
          name: 'AES-GCM',
          length: 256
        },
        rsa: {
          name: 'RSA-OAEP',
          modulusLength: 4096,
          publicExponent: new Uint8Array([1, 0, 1]),
          hash: 'SHA-256'
        },
        ecdsa: {
          name: 'ECDSA',
          namedCurve: 'P-384'
        }
      };
    }
  
    // Key Management
    async generateSymmetricKey() {
      try {
        return await this.subtle.generateKey(
          this.algorithms.aes,
          true, // exportierbar
          ['encrypt', 'decrypt']
        );
      } catch (error) {
        throw new Error(`Fehler bei Schlüsselgenerierung: ${error.message}`);
      }
    }
  
    async generateKeyPair() {
      try {
        return await this.subtle.generateKey(
          {
            ...this.algorithms.rsa,
            hash: { name: this.algorithms.rsa.hash }
          },
          true,
          ['encrypt', 'decrypt', 'sign', 'verify']
        );
      } catch (error) {
        throw new Error(`Fehler bei Schlüsselpaar-Generierung: ${error.message}`);
      }
    }
  
    async exportKey(key, format = 'jwk') {
      try {
        return await this.subtle.exportKey(format, key);
      } catch (error) {
        throw new Error(`Fehler beim Exportieren des Schlüssels: ${error.message}`);
      }
    }
  
    async importKey(keyData, format = 'jwk', algorithm, usages) {
      try {
        return await this.subtle.importKey(
          format,
          keyData,
          algorithm,
          true,
          usages
        );
      } catch (error) {
        throw new Error(`Fehler beim Importieren des Schlüssels: ${error.message}`);
      }
    }
  
    // Verschlüsselung/Entschlüsselung
    async encrypt(data, key) {
      try {
        const iv = this.crypto.getRandomValues(new Uint8Array(12));
        const encodedData = new TextEncoder().encode(data);
        
        const encryptedData = await this.subtle.encrypt(
          {
            name: this.algorithms.aes.name,
            iv
          },
          key,
          encodedData
        );
  
        return {
          encryptedData,
          iv
        };
      } catch (error) {
        throw new Error(`Verschlüsselungsfehler: ${error.message}`);
      }
    }
  
    async decrypt(encryptedData, key, iv) {
      try {
        const decryptedData = await this.subtle.decrypt(
          {
            name: this.algorithms.aes.name,
            iv
          },
          key,
          encryptedData
        );
  
        return new TextDecoder().decode(decryptedData);
      } catch (error) {
        throw new Error(`Entschlüsselungsfehler: ${error.message}`);
      }
    }
  
    // Digitale Signaturen
    async sign(data, privateKey) {
      try {
        const encodedData = new TextEncoder().encode(data);
        return await this.subtle.sign(
          {
            name: this.algorithms.ecdsa.name,
            hash: { name: 'SHA-384' }
          },
          privateKey,
          encodedData
        );
      } catch (error) {
        throw new Error(`Signierfehler: ${error.message}`);
      }
    }
  
    async verify(signature, data, publicKey) {
      try {
        const encodedData = new TextEncoder().encode(data);
        return await this.subtle.verify(
          {
            name: this.algorithms.ecdsa.name,
            hash: { name: 'SHA-384' }
          },
          publicKey,
          signature,
          encodedData
        );
      } catch (error) {
        throw new Error(`Verifikationsfehler: ${error.message}`);
      }
    }
  
    // Secure Random
    getRandomValues(length) {
      return this.crypto.getRandomValues(new Uint8Array(length));
    }
  
    async generateRandomString(length) {
      const charset = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
      const randomValues = this.getRandomValues(length);
      let result = '';
      
      for (let i = 0; i < length; i++) {
        result += charset[randomValues[i] % charset.length];
      }
      
      return result;
    }
  
    // Key Rotation
    async rotateSymmetricKey(oldKey) {
      try {
        // Neuen Schlüssel generieren
        const newKey = await this.generateSymmetricKey();
        
        // Alten Schlüssel exportieren (für Backup/Archivierung)
        const exportedOldKey = await this.exportKey(oldKey);
        
        return {
          newKey,
          exportedOldKey
        };
      } catch (error) {
        throw new Error(`Fehler bei Schlüsselrotation: ${error.message}`);
      }
    }
  
    async rotateKeyPair(oldKeyPair) {
      try {
        // Neues Schlüsselpaar generieren
        const newKeyPair = await this.generateKeyPair();
        
        // Altes Schlüsselpaar exportieren (für Backup/Archivierung)
        const exportedOldPublicKey = await this.exportKey(oldKeyPair.publicKey);
        const exportedOldPrivateKey = await this.exportKey(oldKeyPair.privateKey);
        
        return {
          newKeyPair,
          oldKeyPair: {
            publicKey: exportedOldPublicKey,
            privateKey: exportedOldPrivateKey
          }
        };
      } catch (error) {
        throw new Error(`Fehler bei Schlüsselpaar-Rotation: ${error.message}`);
      }
    }
  }
  
  // Beispielnutzung:
  async function example() {
    const crypto = new CryptoWrapper();
    
    // Symmetrische Verschlüsselung
    const key = await crypto.generateSymmetricKey();
    const data = 'Geheime Nachricht';
    
    const encrypted = await crypto.encrypt(data, key);
    const decrypted = await crypto.decrypt(encrypted.encryptedData, key, encrypted.iv);
    
    console.log('Entschlüsselte Nachricht:', decrypted);
    
    // Digitale Signatur
    const keyPair = await crypto.generateKeyPair();
    const signature = await crypto.sign(data, keyPair.privateKey);
    const isValid = await crypto.verify(signature, data, keyPair.publicKey);
    
    console.log('Signatur gültig:', isValid);
    
    // Schlüsselrotation
    const rotatedKey = await crypto.rotateSymmetricKey(key);
    console.log('Neuer Schlüssel generiert');
  }