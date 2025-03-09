import os
import json
import yaml
import base64
from typing import Any, Dict, Optional
from cryptography.fernet import Fernet
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
import logging

class ConfigurationManager:
    def __init__(self, encryption_key: str):
        """
        Initialisiert den ConfigurationManager mit einem Verschlüsselungsschlüssel.
        
        Args:
            encryption_key: Schlüssel für die Verschlüsselung sensitiver Daten
        """
        self._config: Dict[str, Any] = {}
        self._sensitive_keys: set = set()
        self._setup_encryption(encryption_key)
        self._setup_logging()
    
    def _setup_encryption(self, encryption_key: str) -> None:
        """Initialisiert die Verschlüsselung mit einem sicheren Schlüssel."""
        kdf = PBKDF2HMAC(
            algorithm=hashes.SHA256(),
            length=32,
            salt=b'ConfigManagerSalt',  # In Produktion: Verwende einen sicheren, zufälligen Salt
            iterations=100000,
        )
        key = base64.urlsafe_b64encode(kdf.derive(encryption_key.encode()))
        self._cipher = Fernet(key)

    def _setup_logging(self) -> None:
        """Konfiguriert sicheres Logging ohne sensitive Daten."""
        logging.basicConfig(
            level=logging.INFO,
            format='%(asctime)s - %(levelname)s - %(message)s'
        )
        self.logger = logging.getLogger('ConfigManager')

    def load_environment_variables(self, prefix: str = '') -> None:
        """
        Lädt Umgebungsvariablen mit optionalem Prefix.
        
        Args:
            prefix: Optional prefix für die zu ladenden Umgebungsvariablen
        """
        for key, value in os.environ.items():
            if prefix and not key.startswith(prefix):
                continue
            
            config_key = key[len(prefix):] if prefix else key
            self._config[config_key] = value
            self.logger.info(f"Loaded environment variable: {self._mask_sensitive_value(config_key)}")

    def load_json_file(self, filepath: str) -> None:
        """
        Lädt Konfiguration aus einer JSON-Datei.
        
        Args:
            filepath: Pfad zur JSON-Konfigurationsdatei
        """
        try:
            with open(filepath, 'r') as f:
                config_data = json.load(f)
            self._config.update(config_data)
            self.logger.info(f"Loaded configuration from JSON file: {filepath}")
        except Exception as e:
            self.logger.error(f"Error loading JSON configuration: {str(e)}")
            raise

    def load_yaml_file(self, filepath: str) -> None:
        """
        Lädt Konfiguration aus einer YAML-Datei.
        
        Args:
            filepath: Pfad zur YAML-Konfigurationsdatei
        """
        try:
            with open(filepath, 'r') as f:
                config_data = yaml.safe_load(f)
            self._config.update(config_data)
            self.logger.info(f"Loaded configuration from YAML file: {filepath}")
        except Exception as e:
            self.logger.error(f"Error loading YAML configuration: {str(e)}")
            raise

    def mark_as_sensitive(self, key: str) -> None:
        """
        Markiert einen Konfigurationswert als sensitiv.
        
        Args:
            key: Schlüssel des zu schützenden Wertes
        """
        if key in self._config:
            self._sensitive_keys.add(key)
            self._encrypt_value(key)
            self.logger.info(f"Marked '{key}' as sensitive")

    def _encrypt_value(self, key: str) -> None:
        """Verschlüsselt einen Wert im Speicher."""
        if key in self._config and key in self._sensitive_keys:
            value = str(self._config[key]).encode()
            self._config[key] = self._cipher.encrypt(value)

    def _decrypt_value(self, encrypted_value: bytes) -> str:
        """Entschlüsselt einen verschlüsselten Wert."""
        return self._cipher.decrypt(encrypted_value).decode()

    def get(self, key: str, default: Any = None) -> Any:
        """
        Holt einen Konfigurationswert.
        
        Args:
            key: Schlüssel des gewünschten Wertes
            default: Standardwert falls der Schlüssel nicht existiert
        
        Returns:
            Den Konfigurationswert oder den Standardwert
        """
        value = self._config.get(key, default)
        if key in self._sensitive_keys and value is not None:
            return self._decrypt_value(value)
        return value

    def _mask_sensitive_value(self, key: str) -> str:
        """Maskiert sensitive Werte für das Logging."""
        return f"{key}=****" if key in self._sensitive_keys else f"{key}={self._config.get(key)}"

    def __str__(self) -> str:
        """String-Repräsentation mit maskierten sensitiven Werten."""
        masked_config = {
            k: "****" if k in self._sensitive_keys else v
            for k, v in self._config.items()
        }
        return f"ConfigurationManager(config={masked_config})"


# Beispielnutzung:
if __name__ == "__main__":
    # Erstelle eine Instanz mit einem sicheren Schlüssel
    config_manager = ConfigurationManager("your-secure-encryption-key")

    # Lade Umgebungsvariablen
    config_manager.load_environment_variables("APP_")

    # Lade Konfiguration aus Dateien
    config_manager.load_json_file("config.json")
    config_manager.load_yaml_file("config.yaml")

    # Markiere sensitive Werte
    config_manager.mark_as_sensitive("DATABASE_PASSWORD")
    config_manager.mark_as_sensitive("API_KEY")

    # Zugriff auf Konfigurationswerte
    db_host = config_manager.get("DATABASE_HOST", "localhost")
    db_password = config_manager.get("DATABASE_PASSWORD")  # Wird automatisch entschlüsselt

    # Logging zeigt keine sensitiven Werte
    print(config_manager)  # Sensitive Werte werden als **** angezeigt