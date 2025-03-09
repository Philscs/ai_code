import os
import shutil
import hashlib
import tempfile
import boto3
from google.cloud import storage
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.padding import PKCS7
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC

# Konfiguration
BACKUP_SOURCE = "/path/to/source"
BACKUP_DESTINATION = "/path/to/backup"
CLOUD_CONFIG = {
    "aws": {
        "bucket_name": "your-s3-bucket",
        "access_key": "your-access-key",
        "secret_key": "your-secret-key",
    },
    "gcp": {
        "bucket_name": "your-gcp-bucket",
        "credentials_file": "/path/to/credentials.json",
    },
}
ENCRYPTION_KEY = b"your-secure-key"  # Verwende eine sichere Schlüsselquelle
SALT = b"your-salt"

# Funktionen
def derive_key(password: bytes, salt: bytes) -> bytes:
    kdf = PBKDF2HMAC(
        algorithm=hashes.SHA256(),
        length=32,
        salt=salt,
        iterations=100000,
        backend=default_backend(),
    )
    return kdf.derive(password)

def encrypt_file(file_path: str, key: bytes) -> str:
    with open(file_path, "rb") as f:
        data = f.read()

    cipher = Cipher(algorithms.AES(key), modes.CFB(os.urandom(16)), backend=default_backend())
    encryptor = cipher.encryptor()
    padded_data = PKCS7(128).padder().update(data) + PKCS7(128).padder().finalize()
    encrypted_data = encryptor.update(padded_data) + encryptor.finalize()

    temp_file = tempfile.NamedTemporaryFile(delete=False)
    with open(temp_file.name, "wb") as f:
        f.write(cipher.algorithm.block_size.to_bytes(16, "big"))  # Initialization Vector
        f.write(encrypted_data)
    return temp_file.name

def secure_delete(file_path: str):
    with open(file_path, "ba+") as f:
        length = f.tell()
        f.seek(0)
        f.write(os.urandom(length))
    os.remove(file_path)

def incremental_backup(source: str, destination: str):
    for root, _, files in os.walk(source):
        for file in files:
            src_path = os.path.join(root, file)
            rel_path = os.path.relpath(src_path, source)
            dest_path = os.path.join(destination, rel_path)

            if not os.path.exists(dest_path) or (
                os.path.getmtime(src_path) > os.path.getmtime(dest_path)
            ):
                os.makedirs(os.path.dirname(dest_path), exist_ok=True)
                shutil.copy2(src_path, dest_path)


def sync_to_aws(file_path: str, config: dict):
    s3 = boto3.client(
        "s3",
        aws_access_key_id=config["access_key"],
        aws_secret_access_key=config["secret_key"],
    )
    s3.upload_file(file_path, config["bucket_name"], os.path.basename(file_path))

def sync_to_gcp(file_path: str, config: dict):
    client = storage.Client.from_service_account_json(config["credentials_file"])
    bucket = client.get_bucket(config["bucket_name"])
    blob = bucket.blob(os.path.basename(file_path))
    blob.upload_from_filename(file_path)

# Hauptprogramm
def main():
    key = derive_key(ENCRYPTION_KEY, SALT)

    # Inkrementelles Backup
    incremental_backup(BACKUP_SOURCE, BACKUP_DESTINATION)

    for root, _, files in os.walk(BACKUP_DESTINATION):
        for file in files:
            file_path = os.path.join(root, file)
            encrypted_file = encrypt_file(file_path, key)

            # Synchronisierung mit Clouds
            sync_to_aws(encrypted_file, CLOUD_CONFIG["aws"])
            sync_to_gcp(encrypted_file, CLOUD_CONFIG["gcp"])

            # Temporäre Datei sicher löschen
            secure_delete(encrypted_file)

if __name__ == "__main__":
    main()
