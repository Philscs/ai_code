import os
import json
from cryptography.hazmat.primitives.asymmetric import rsa, padding
from cryptography.hazmat.primitives import serialization, hashes
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import padding as sym_padding
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from base64 import urlsafe_b64encode, urlsafe_b64decode

class EncryptionSystem:
    def __init__(self):
        self.key_storage = "keys.json"
        self.keys = self.load_keys()

    def load_keys(self):
        if os.path.exists(self.key_storage):
            with open(self.key_storage, "r") as file:
                return json.load(file)
        return {}

    def save_keys(self):
        with open(self.key_storage, "w") as file:
            json.dump(self.keys, file)

    def generate_aes_key(self, key_name, password):
        salt = os.urandom(16)
        kdf = PBKDF2HMAC(
            algorithm=hashes.SHA256(),
            length=32,
            salt=salt,
            iterations=100000,
            backend=default_backend()
        )
        key = kdf.derive(password.encode())
        self.keys[key_name] = {
            "type": "AES",
            "key": urlsafe_b64encode(key).decode(),
            "salt": urlsafe_b64encode(salt).decode()
        }
        self.save_keys()

    def generate_rsa_keypair(self, key_name):
        private_key = rsa.generate_private_key(
            public_exponent=65537,
            key_size=2048,
            backend=default_backend()
        )
        public_key = private_key.public_key()

        private_key_pem = private_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.PKCS8,
            encryption_algorithm=serialization.NoEncryption()
        )

        public_key_pem = public_key.public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        )

        self.keys[key_name] = {
            "type": "RSA",
            "private_key": private_key_pem.decode(),
            "public_key": public_key_pem.decode()
        }
        self.save_keys()

    def get_key(self, key_name):
        return self.keys.get(key_name)

    def encrypt_file_aes(self, file_path, key_name):
        key_data = self.get_key(key_name)
        if not key_data or key_data["type"] != "AES":
            raise ValueError("Invalid AES key name")

        key = urlsafe_b64decode(key_data["key"])
        iv = os.urandom(16)

        with open(file_path, "rb") as f:
            plaintext = f.read()

        padder = sym_padding.PKCS7(128).padder()
        padded_data = padder.update(plaintext) + padder.finalize()

        cipher = Cipher(algorithms.AES(key), modes.CBC(iv), backend=default_backend())
        encryptor = cipher.encryptor()
        ciphertext = encryptor.update(padded_data) + encryptor.finalize()

        with open(file_path + ".enc", "wb") as f:
            f.write(iv + ciphertext)

    def decrypt_file_aes(self, file_path, key_name):
        key_data = self.get_key(key_name)
        if not key_data or key_data["type"] != "AES":
            raise ValueError("Invalid AES key name")

        key = urlsafe_b64decode(key_data["key"])

        with open(file_path, "rb") as f:
            iv = f.read(16)
            ciphertext = f.read()

        cipher = Cipher(algorithms.AES(key), modes.CBC(iv), backend=default_backend())
        decryptor = cipher.decryptor()
        padded_plaintext = decryptor.update(ciphertext) + decryptor.finalize()

        unpadder = sym_padding.PKCS7(128).unpadder()
        plaintext = unpadder.update(padded_plaintext) + unpadder.finalize()

        with open(file_path.replace(".enc", ""), "wb") as f:
            f.write(plaintext)

    def encrypt_file_rsa(self, file_path, key_name):
        key_data = self.get_key(key_name)
        if not key_data or key_data["type"] != "RSA":
            raise ValueError("Invalid RSA key name")

        public_key = serialization.load_pem_public_key(
            key_data["public_key"].encode(), backend=default_backend()
        )

        with open(file_path, "rb") as f:
            plaintext = f.read()

        ciphertext = public_key.encrypt(
            plaintext,
            padding.OAEP(
                mgf=padding.MGF1(algorithm=hashes.SHA256()),
                algorithm=hashes.SHA256(),
                label=None
            )
        )

        with open(file_path + ".enc", "wb") as f:
            f.write(ciphertext)

    def decrypt_file_rsa(self, file_path, key_name):
        key_data = self.get_key(key_name)
        if not key_data or key_data["type"] != "RSA":
            raise ValueError("Invalid RSA key name")

        private_key = serialization.load_pem_private_key(
            key_data["private_key"].encode(), password=None, backend=default_backend()
        )

        with open(file_path, "rb") as f:
            ciphertext = f.read()

        plaintext = private_key.decrypt(
            ciphertext,
            padding.OAEP(
                mgf=padding.MGF1(algorithm=hashes.SHA256()),
                algorithm=hashes.SHA256(),
                label=None
            )
        )

        with open(file_path.replace(".enc", ""), "wb") as f:
            f.write(plaintext)

    def encrypt_folder(self, folder_path, key_name, algorithm="AES"):
        for root, _, files in os.walk(folder_path):
            for file in files:
                file_path = os.path.join(root, file)
                if algorithm == "AES":
                    self.encrypt_file_aes(file_path, key_name)
                elif algorithm == "RSA":
                    self.encrypt_file_rsa(file_path, key_name)

    def decrypt_folder(self, folder_path, key_name, algorithm="AES"):
        for root, _, files in os.walk(folder_path):
            for file in files:
                file_path = os.path.join(root, file)
                if file.endswith(".enc"):
                    if algorithm == "AES":
                        self.decrypt_file_aes(file_path, key_name)
                    elif algorithm == "RSA":
                        self.decrypt_file_rsa(file_path, key_name)

# Beispiel:
# system = EncryptionSystem()
# system.generate_aes_key("my_aes_key", "password123")
# system.encrypt_file_aes("example.txt", "my_aes_key")
# system.decrypt_file_aes("example.txt.enc", "my_aes_key")
