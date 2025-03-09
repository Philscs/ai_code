import os
from cryptography.fernet import Fernet
import logging

# Einstellungen für den Manager
class ConfigManager:
    def __init__(self, config_source='default'):
        self.config_source = config_source

    # Verschlüsselung der Konfiguration mit einem sicheren Schlüssel
    def _encrypt_config(self):
        try:
            with open('config.json') as f:
                config_str = f.read()
            fernet_key = Fernet.generate_key()
            cipher_suite = Fernet(fernet_key)
            encrypted_config = cipher_suite.encrypt(config_str.encode())
            # Speichern des verschlüsselten Konfigurationsinhalts in einer Datei
            with open('encrypted_config.json', 'wb') as f:
                f.write(fernet_key)
                f.write(b'\n')
                f.write(encrypted_config)
        except FileNotFoundError:
            pass

    # Ausgabe der Konfiguration aus einer Datei, wobei sensible Werte verschlüsselt werden
    def _get_config(self):
        try:
            with open('encrypted_config.json') as f:
                fernet_key = f.read()
                encrypted_config = f.read(1024)
            cipher_suite = Fernet(fernet_key)
            plain_config_str = cipher_suite.decrypt(encrypted_config).decode()
            # Lese die Konfiguration aus einer Datei
            with open('config.json') as config_file:
                config_str = config_file.read()
            if plain_config_str != config_str:
                logging.warning("Konfigurationsdatei wurde verändert, während des Lastens!")
            return plain_config_str + '\n' + config_str
        except FileNotFoundError:
            pass

    # Lese die Konfiguration aus einer Datei und integriere Umgebungsvariablen
    def get_config(self):
        try:
            with open('config.json') as f:
                config_str = f.read()
            # Integriere Umgebungsvariablen in der Konfiguration
            for key, value in os.environ.items():
                if self.config_source == 'default' and key.startswith(f'{self.config_source}_'):
                    new_key = key[len(self.config_source)+1:]
                    config_str = config_str.replace(key, f'{new_key}={value}')
            return config_str
        except FileNotFoundError:
            pass

    # Lese die Konfiguration aus einer Datei und integriere Umgebungsvariablen
    def get_sensitive_config(self):
        try:
            with open('config.json') as f:
                config_str = f.read()
            # Integriere Umgebungsvariablen in der Konfiguration
            for key, value in os.environ.items():
                if self.config_source == 'default' and key.startswith(f'{self.config_source}_'):
                    new_key = key[len(self.config_source)+1:]
                    config_str = config_str.replace(key, f'{new_key}={value}')
            # Verschlüsseln der Konfigurationswerte mit einem sicheren Schlüssel
            try:
                with open('encrypted_config.json') as f:
                    fernet_key = f.read()
                    encrypted_config = f.read(1024)
                cipher_suite = Fernet(fernet_key)
                plain_config_str = cipher_suite.decrypt(encrypted_config).decode()
                config_str = plain_config_str
            except FileNotFoundError:
                pass
            return config_str
        except FileNotFoundError:
            pass

# Einstellungen für den Manager
config_manager = ConfigManager()

# Lese die Konfiguration aus einer Datei und integriere Umgebungsvariablen
config_str = config_manager.get_config()
print(config_str)

# Lese die sensiblen Konfigurationswerte aus einer Datei und integriere Umgebungsvariablen
sensitive_config_str = config_manager.get_sensitive_config()
print(sensitive_config_str)
