import logging
import logging.handlers
import os
from cryptography.fernet import Fernet

# Configure logging
log_file = '/path/to/log/file.log'
log_rotation = 5  # Number of log files to keep
log_level = logging.INFO

# Configure encryption
key = b'your-encryption-key'
cipher_suite = Fernet(key)

# Create logger
logger = logging.getLogger('system_events')
logger.setLevel(log_level)

# Create file handler for log rotation
file_handler = logging.handlers.RotatingFileHandler(log_file, maxBytes=1024, backupCount=log_rotation)
logger.addHandler(file_handler)

# Create formatter
formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
file_handler.setFormatter(formatter)

# Log an event
logger.info('System event: Something happened')

# Encrypt sensitive information
sensitive_info = 'Sensitive information'
encrypted_info = cipher_suite.encrypt(sensitive_info.encode())

# Decrypt sensitive information
decrypted_info = cipher_suite.decrypt(encrypted_info).decode()

# Clean up old log files
log_dir = os.path.dirname(log_file)
for file_name in os.listdir(log_dir):
    if file_name.startswith(os.path.basename(log_file)) and file_name != os.path.basename(log_file):
        os.remove(os.path.join(log_dir, file_name))
