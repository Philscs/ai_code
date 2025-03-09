from cryptography.fernet import Fernet
from cryptography.hazmat.primitives import hashes, padding
from cryptography.hazmat.primitives.asymmetric import rsa, padding as asym_padding
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
import os
import json
from pathlib import Path
from base64 import b64encode, b64decode
from typing import Union, Dict, Optional

class KeyManager:
    def __init__(self, keys_file: str = "keys.json"):
        self.keys_file = keys_file
        self.keys = self._load_keys()
    
    def _load_keys(self) -> Dict:
        """Lädt gespeicherte Schlüssel aus der JSON-Datei"""
        if os.path.exists(self.keys_file):
            with open(self.keys_file, 'r') as f:
                return json.load(f)
        return {}
    
    def save_keys(self):
        """Speichert Schlüssel in der JSON-Datei"""
        with open(self.keys_file, 'w') as f:
            json.dump(self.keys, f)
    
    def generate_key(self, name: str, algorithm: str = "AES"):
        """Generiert einen neuen Schlüssel für den angegebenen Algorithmus"""
        if algorithm == "AES":
            key = os.urandom(32)  # 256-bit Schlüssel
        elif algorithm == "RSA":
            private_key = rsa.generate_private_key(
                public_exponent=65537,
                key_size=2048
            )
            key = {
                'private': private_key.private_bytes(...),
                'public': private_key.public_key().public_bytes(...)
            }
        
        self.keys[name] = {
            'algorithm': algorithm,
            'key': b64encode(key).decode() if isinstance(key, bytes) else key
        }
        self.save_keys()
        return key

    def get_key(self, name: str) -> Union[bytes, Dict]:
        """Gibt einen gespeicherten Schlüssel zurück"""
        if name not in self.keys:
            raise KeyError(f"Schlüssel '{name}' nicht gefunden")
        key_data = self.keys[name]
        key = key_data['key']
        return b64decode(key) if isinstance(key, str) else key

class EncryptionSystem:
    def __init__(self):
        self.key_manager = KeyManager()
    
    def encrypt_file(self, file_path: str, key_name: str, algorithm: str = "AES") -> None:
        """Verschlüsselt eine Datei mit dem angegebenen Algorithmus und Schlüssel"""
        key = self.key_manager.get_key(key_name)
        
        with open(file_path, 'rb') as f:
            data = f.read()
        
        if algorithm == "AES":
            iv = os.urandom(16)
            cipher = Cipher(algorithms.AES(key), modes.CBC(iv))
            encryptor = cipher.encryptor()
            
            # Padding hinzufügen
            padder = padding.PKCS7(128).padder()
            padded_data = padder.update(data) + padder.finalize()
            
            # Verschlüsseln
            encrypted_data = encryptor.update(padded_data) + encryptor.finalize()
            
            # IV an verschlüsselte Daten anhängen
            final_data = iv + encrypted_data
            
        elif algorithm == "RSA":
            public_key = key['public']
            encrypted_data = public_key.encrypt(
                data,
                asym_padding.OAEP(
                    mgf=asym_padding.MGF1(algorithm=hashes.SHA256()),
                    algorithm=hashes.SHA256(),
                    label=None
                )
            )
            final_data = encrypted_data
        
        # Verschlüsselte Datei speichern
        encrypted_file = file_path + '.encrypted'
        with open(encrypted_file, 'wb') as f:
            f.write(final_data)

    def decrypt_file(self, file_path: str, key_name: str, algorithm: str = "AES") -> None:
        """Entschlüsselt eine Datei mit dem angegebenen Algorithmus und Schlüssel"""
        key = self.key_manager.get_key(key_name)
        
        with open(file_path, 'rb') as f:
            data = f.read()
        
        if algorithm == "AES":
            # IV extrahieren (erste 16 Bytes)
            iv = data[:16]
            encrypted_data = data[16:]
            
            cipher = Cipher(algorithms.AES(key), modes.CBC(iv))
            decryptor = cipher.decryptor()
            
            # Entschlüsseln
            padded_data = decryptor.update(encrypted_data) + decryptor.finalize()
            
            # Padding entfernen
            unpadder = padding.PKCS7(128).unpadder()
            decrypted_data = unpadder.update(padded_data) + unpadder.finalize()
            
        elif algorithm == "RSA":
            private_key = key['private']
            decrypted_data = private_key.decrypt(
                data,
                asym_padding.OAEP(
                    mgf=asym_padding.MGF1(algorithm=hashes.SHA256()),
                    algorithm=hashes.SHA256(),
                    label=None
                )
            )
        
        # Entschlüsselte Datei speichern
        decrypted_file = file_path.replace('.encrypted', '.decrypted')
        with open(decrypted_file, 'wb') as f:
            f.write(decrypted_data)

    def encrypt_directory(self, dir_path: str, key_name: str, algorithm: str = "AES") -> None:
        """Verschlüsselt alle Dateien in einem Verzeichnis rekursiv"""
        dir_path = Path(dir_path)
        for file_path in dir_path.rglob('*'):
            if file_path.is_file() and not file_path.suffix == '.encrypted':
                self.encrypt_file(str(file_path), key_name, algorithm)

    def decrypt_directory(self, dir_path: str, key_name: str, algorithm: str = "AES") -> None:
        """Entschlüsselt alle verschlüsselten Dateien in einem Verzeichnis rekursiv"""
        dir_path = Path(dir_path)
        for file_path in dir_path.rglob('*.encrypted'):
            if file_path.is_file():
                self.decrypt_file(str(file_path), key_name, algorithm)

# Beispielverwendung:
if __name__ == "__main__":
    # Verschlüsselungssystem initialisieren
    encryption_system = EncryptionSystem()
    
    # Neuen AES-Schlüssel generieren
    encryption_system.key_manager.generate_key("mein_aes_schluessel", "AES")
    
    # Datei verschlüsseln
    encryption_system.encrypt_file("beispiel.txt", "mein_aes_schluessel", "AES")
    
    # Datei entschlüsseln
    encryption_system.decrypt_file("beispiel.txt.encrypted", "mein_aes_schluessel", "AES")
    
    # Verzeichnis verschlüsseln
    encryption_system.encrypt_directory("vertrauliche_dokumente", "mein_aes_schluessel", "AES")