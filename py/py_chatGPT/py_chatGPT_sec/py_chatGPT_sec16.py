import os
import json
from cryptography.fernet import Fernet
from typing import Any, Dict

class ConfigManager:
    def __init__(self, encryption_key: str):
        self.encryption_key = encryption_key
        self.cipher = Fernet(encryption_key)
        self.config: Dict[str, Any] = {}

    def load_from_env(self, prefix: str = "CONFIG_") -> None:
        """Load configuration from environment variables with a given prefix."""
        for key, value in os.environ.items():
            if key.startswith(prefix):
                config_key = key[len(prefix):]
                self.config[config_key] = value

    def load_from_file(self, file_path: str) -> None:
        """Load configuration from a JSON file."""
        try:
            with open(file_path, 'r') as file:
                file_config = json.load(file)
                self.config.update(file_config)
        except Exception as e:
            raise RuntimeError(f"Failed to load configuration file: {e}")

    def get(self, key: str, default: Any = None) -> Any:
        """Retrieve a configuration value, falling back to a default if not found."""
        return self.config.get(key, default)

    def set(self, key: str, value: Any) -> None:
        """Set a configuration value."""
        self.config[key] = value

    def encrypt_sensitive_value(self, value: str) -> str:
        """Encrypt a sensitive configuration value."""
        return self.cipher.encrypt(value.encode()).decode()

    def decrypt_sensitive_value(self, encrypted_value: str) -> str:
        """Decrypt a sensitive configuration value."""
        return self.cipher.decrypt(encrypted_value.encode()).decode()

    def set_sensitive(self, key: str, value: str) -> None:
        """Store a sensitive configuration value in encrypted form."""
        encrypted_value = self.encrypt_sensitive_value(value)
        self.config[key] = encrypted_value

    def get_sensitive(self, key: str) -> str:
        """Retrieve and decrypt a sensitive configuration value."""
        encrypted_value = self.config.get(key)
        if not encrypted_value:
            raise KeyError(f"Sensitive configuration key '{key}' not found.")
        return self.decrypt_sensitive_value(encrypted_value)

    def to_safe_dict(self) -> Dict[str, Any]:
        """Return a dictionary representation of the configuration with sensitive values masked."""
        safe_config = {}
        for key, value in self.config.items():
            if isinstance(value, str) and self.is_encrypted(value):
                safe_config[key] = "<ENCRYPTED>"
            else:
                safe_config[key] = value
        return safe_config

    def is_encrypted(self, value: str) -> bool:
        """Check if a value is encrypted."""
        try:
            self.cipher.decrypt(value.encode())
            return True
        except Exception:
            return False

    def log_safe_config(self):
        """Log the configuration with sensitive values masked."""
        safe_config = self.to_safe_dict()
        print(json.dumps(safe_config, indent=2))

# Beispiel-Nutzung
if __name__ == "__main__":
    # Schlüssel für die Verschlüsselung generieren (dies sollte sicher gespeichert werden!)
    encryption_key = Fernet.generate_key()
    config_manager = ConfigManager(encryption_key=encryption_key)

    # Beispiel für das Laden aus Umgebungsvariablen
    os.environ["CONFIG_API_KEY"] = "my_api_key"
    config_manager.load_from_env()

    # Sensitiven Wert setzen und abrufen
    config_manager.set_sensitive("database_password", "super_secure_password")
    print("Decrypted sensitive value:", config_manager.get_sensitive("database_password"))

    # Sichere Konfiguration anzeigen
    config_manager.log_safe_config()
