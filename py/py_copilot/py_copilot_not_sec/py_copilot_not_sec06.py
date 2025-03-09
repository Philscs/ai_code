import os
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives import serialization, hashes
from cryptography.hazmat.backends import default_backend


class EncryptionSystem:
    def __init__(self):
        self.key = None

    def generate_rsa_key(self):
        private_key = rsa.generate_private_key(
            public_exponent=65537,
            key_size=2048,
            backend=default_backend()
        )
        self.key = private_key

    def save_rsa_key(self, filename):
        if self.key is None:
            raise ValueError("No key generated")
        with open(filename, "wb") as key_file:
            key_file.write(self.key.private_bytes(
                encoding=serialization.Encoding.PEM,
                format=serialization.PrivateFormat.PKCS8,
                encryption_algorithm=serialization.NoEncryption()
            ))

    def load_rsa_key(self, filename):
        with open(filename, "rb") as key_file:
            self.key = serialization.load_pem_private_key(
                key_file.read(),
                password=None,
                backend=default_backend()
            )

    def encrypt_file_aes(self, filename, key):
        iv = os.urandom(16)
        cipher = Cipher(algorithms.AES(key), modes.CBC(iv), backend=default_backend())
        encryptor = cipher.encryptor()
        with open(filename, "rb") as file:
            plaintext = file.read()
        ciphertext = encryptor.update(plaintext) + encryptor.finalize()
        with open(filename + ".enc", "wb") as file:
            file.write(iv + ciphertext)

    def decrypt_file_aes(self, filename, key):
        with open(filename, "rb") as file:
            ciphertext = file.read()
        iv = ciphertext[:16]
        ciphertext = ciphertext[16:]
        cipher = Cipher(algorithms.AES(key), modes.CBC(iv), backend=default_backend())
        decryptor = cipher.decryptor()
        plaintext = decryptor.update(ciphertext) + decryptor.finalize()
        with open(filename[:-4], "wb") as file:
            file.write(plaintext)

    def encrypt_folder_aes(self, foldername, key):
        for root, dirs, files in os.walk(foldername):
            for file in files:
                self.encrypt_file_aes(os.path.join(root, file), key)

    def decrypt_folder_aes(self, foldername, key):
        for root, dirs, files in os.walk(foldername):
            for file in files:
                if file.endswith(".enc"):
                    self.decrypt_file_aes(os.path.join(root, file), key)
