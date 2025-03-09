import os
from cryptography.fernet import Fernet

class ConfigurationManager:
    def __init__(self, config_sources):
        self.config_sources = config_sources
        self.encrypted_values = {}

    def get_config_value(self, key):
        if key in self.encrypted_values:
            return self.decrypt_value(self.encrypted_values[key])

        for source in self.config_sources:
            if key in source:
                value = source[key]
                if self.is_sensitive(key):
                    self.encrypted_values[key] = self.encrypt_value(value)
                    return self.decrypt_value(self.encrypted_values[key])
                return value

        return None

    def is_sensitive(self, key):
        # Add your logic to determine if a key is sensitive or not
        # For example, you can have a list of sensitive keys or a naming convention
        sensitive_keys = ['password', 'secret']
        return key.lower() in sensitive_keys

    def encrypt_value(self, value):
        # Add your encryption logic here
        key = Fernet.generate_key()
        cipher_suite = Fernet(key)
        encrypted_value = cipher_suite.encrypt(value.encode())
        return encrypted_value

    def decrypt_value(self, encrypted_value):
        # Add your decryption logic here
        key = Fernet.generate_key()
        cipher_suite = Fernet(key)
        decrypted_value = cipher_suite.decrypt(encrypted_value).decode()
        return decrypted_value

# Example usage
config_sources = [
    {'database_url': 'db.example.com'},
    {'api_key': 'my_api_key'},
    os.environ
]

config_manager = ConfigurationManager(config_sources)
database_url = config_manager.get_config_value('database_url')
api_key = config_manager.get_config_value('api_key')

print(database_url)
print(api_key)
