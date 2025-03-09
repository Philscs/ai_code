import os
import hashlib
import json
import shutil
from datetime import datetime
from pathlib import Path
from cryptography.fernet import Fernet
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
import boto3
from google.cloud import storage
import logging
import secrets


class SecureBackupSystem:
    def __init__(self, source_dir, backup_dir, encryption_key=None):
        self.source_dir = Path(source_dir)
        self.backup_dir = Path(backup_dir)
        self.manifest_file = self.backup_dir / "backup_manifest.json"
        self.temp_dir = self.backup_dir / "temp"
        self.key_file = self.backup_dir / "key_store.enc"

        logging.basicConfig(
            filename=self.backup_dir / "backup.log",
            level=logging.INFO,
            format='%(asctime)s - %(levelname)s - %(message)s'
        )

        self.encryption_key = self._setup_encryption(encryption_key)

        self.backup_dir.mkdir(exist_ok=True)
        self.temp_dir.mkdir(exist_ok=True)

    def _setup_encryption(self, provided_key=None):
        if provided_key:
            return provided_key

        if self.key_file.exists():
            with open(self.key_file, 'rb') as f:
                return f.read()

        key = AESGCM.generate_key(bit_length=256)

        with open(self.key_file, 'wb') as f:
            f.write(key)

        return key

    def _get_file_hash(self, file_path):
        sha256_hash = hashlib.sha256()
        with open(file_path, "rb") as f:
            for byte_block in iter(lambda: f.read(4096), b""):
                sha256_hash.update(byte_block)
        return sha256_hash.hexdigest()

    def _encrypt_file(self, source_path, dest_path):
        aesgcm = AESGCM(self.encryption_key)
        nonce = os.urandom(12)

        with open(source_path, 'rb') as f:
            data = f.read()

        encrypted_data = aesgcm.encrypt(nonce, data, None)

        with open(dest_path, 'wb') as f:
            f.write(nonce + encrypted_data)

    def _decrypt_file(self, source_path, dest_path):
        aesgcm = AESGCM(self.encryption_key)

        with open(source_path, 'rb') as f:
            nonce = f.read(12)
            encrypted_data = f.read()

        decrypted_data = aesgcm.decrypt(nonce, encrypted_data, None)

        with open(dest_path, 'wb') as f:
            f.write(decrypted_data)

    def _secure_delete(self, path):
        if not os.path.exists(path):
            return

        size = os.path.getsize(path)

        for _ in range(3):
            with open(path, 'wb') as f:
                f.write(secrets.token_bytes(size))

        os.remove(path)

    def create_backup(self):
        try:
            if self.manifest_file.exists():
                with open(self.manifest_file, 'r') as f:
                    manifest = json.load(f)
            else:
                manifest = {'backups': {}}

            backup_time = datetime.now().isoformat()
            current_backup = {
                'files': {},
                'timestamp': backup_time
            }

            for file_path in self.source_dir.rglob('*'):
                if file_path.is_file():
                    rel_path = str(file_path.relative_to(self.source_dir))
                    file_hash = self._get_file_hash(file_path)

                    needs_backup = True
                    if manifest['backups']:
                        last_backup = list(manifest['backups'].values())[-1]
                        if rel_path in last_backup['files']:
                            if last_backup['files'][rel_path]['hash'] == file_hash:
                                needs_backup = False

                    if needs_backup:
                        temp_path = self.temp_dir / f"{file_hash}.enc"
                        backup_path = self.backup_dir / backup_time / rel_path

                        backup_path.parent.mkdir(parents=True, exist_ok=True)

                        self._encrypt_file(file_path, temp_path)
                        shutil.move(str(temp_path), str(backup_path))

                        current_backup['files'][rel_path] = {
                            'hash': file_hash,
                            'backup_path': str(backup_path)
                        }

            manifest['backups'][backup_time] = current_backup
            with open(self.manifest_file, 'w') as f:
                json.dump(manifest, f, indent=4)

            logging.info(f"Backup erfolgreich erstellt: {backup_time}")
            return True

        except Exception as e:
            logging.error(f"Fehler beim Backup-Erstellen: {str(e)}")
            return False
        finally:
            for temp_file in self.temp_dir.glob('*'):
                self._secure_delete(temp_file)

    def sync_to_cloud(self, provider='s3', credentials=None):
        try:
            if provider == 's3':
                s3 = boto3.client('s3',
                                  aws_access_key_id=credentials['access_key'],
                                  aws_secret_access_key=credentials['secret_key']
                                  )

                bucket_name = credentials['bucket']

                for file_path in self.backup_dir.rglob('*'):
                    if file_path.is_file():
                        rel_path = str(file_path.relative_to(self.backup_dir))
                        s3.upload_file(str(file_path), bucket_name, rel_path)

            elif provider == 'gcp':
                storage_client = storage.Client.from_service_account_json(
                    credentials['service_account_path']
                )

                bucket_name = credentials['bucket']
                bucket = storage_client.bucket(bucket_name)

                for file_path in self.backup_dir.rglob('*'):
                    if file_path.is_file():
                        rel_path = str(file_path.relative_to(self.backup_dir))
                        blob = bucket.blob(rel_path)
                        blob.upload_from_filename(str(file_path))

            logging.info(f"Cloud-Synchronisation erfolgreich: {provider}")
            return True

        except Exception as e:
            logging.error(f"Fehler bei Cloud-Synchronisation: {str(e)}")
            return False

    def restore_backup(self, timestamp, restore_path):
        try:
            restore_path = Path(restore_path)
            restore_path.mkdir(parents=True, exist_ok=True)

            with open(self.manifest_file, 'r') as f:
                manifest = json.load(f)

            if timestamp not in manifest['backups']:
                raise ValueError(f"Backup für Zeitstempel {timestamp} nicht gefunden")

            backup = manifest['backups'][timestamp]

            for rel_path, file_info in backup['files'].items():
                source_path = Path(file_info['backup_path'])
                dest_path = restore_path / rel_path

                dest_path.parent.mkdir(parents=True, exist_ok=True)

                temp_path = self.temp_dir / f"restore_{source_path.name}"

                self._decrypt_file(source_path, temp_path)
                shutil.move(str(temp_path), str(dest_path))

            logging.info(f"Backup erfolgreich wiederhergestellt: {timestamp}")
            return True

        except Exception as e:
            logging.error(f"Fehler bei der Wiederherstellung: {str(e)}")
            return False
        finally:
            for temp_file in self.temp_dir.glob('*'):
                self._secure_delete(temp_file)


# Beispiel für die Verwendung
if __name__ == "__main__":
    backup_system = SecureBackupSystem(
        source_dir="/pfad/zu/quellverzeichnis",
        backup_dir="/pfad/zu/backup"
    )

    backup_system.create_backup()

    aws_credentials = {
        'access_key': 'IHRE_ACCESS_KEY',
        'secret_key': 'IHRE_SECRET_KEY',
        'bucket': 'backup-bucket'
    }
    backup_system.sync_to_cloud(provider='s3', credentials=aws_credentials)

    backup_system.restore_backup(
        timestamp="2024-01-05T12:00:00",
        restore_path="/pfad/zur/wiederherstellung"
    )
