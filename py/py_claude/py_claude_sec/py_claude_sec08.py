import logging
import logging.handlers
import json
import hashlib
import base64
from cryptography.fernet import Fernet
from datetime import datetime
import os
from typing import Dict, Any, Optional

class SecureLogger:
    def __init__(self, 
                 log_file: str = "system_events.log",
                 max_bytes: int = 5_000_000,  # 5MB
                 backup_count: int = 5,
                 encryption_key: Optional[str] = None):
        
        # Encryption setup
        self.encryption_key = encryption_key or Fernet.generate_key()
        self.cipher_suite = Fernet(self.encryption_key)
        
        # Configure basic logging
        self.logger = logging.getLogger("SecureLogger")
        self.logger.setLevel(logging.INFO)
        
        # Setup rotating file handler with encryption
        handler = self._create_rotating_handler(log_file, max_bytes, backup_count)
        self.logger.addHandler(handler)
        
        # Alert thresholds configuration
        self.alert_thresholds = {
            "ERROR": 5,  # Alert after 5 errors in time window
            "CRITICAL": 1  # Alert immediately for critical events
        }
        
        # Alert counters
        self.error_count = 0
        self.last_alert_time = datetime.now()
        self.alert_window = 3600  # 1 hour window for counting events
        
    def _create_rotating_handler(self, log_file: str, max_bytes: int, backup_count: int) -> logging.Handler:
        """Create a rotating file handler with encryption."""
        handler = logging.handlers.RotatingFileHandler(
            log_file,
            maxBytes=max_bytes,
            backupCount=backup_count
        )
        handler.setFormatter(logging.Formatter(
            '%(asctime)s - %(levelname)s - %(message)s'
        ))
        return handler
    
    def _encrypt_sensitive_data(self, data: str) -> str:
        """Encrypt sensitive information before logging."""
        encrypted_data = self.cipher_suite.encrypt(data.encode())
        return base64.b64encode(encrypted_data).decode()
    
    def _decrypt_sensitive_data(self, encrypted_data: str) -> str:
        """Decrypt sensitive information from logs."""
        decoded_data = base64.b64decode(encrypted_data)
        decrypted_data = self.cipher_suite.decrypt(decoded_data)
        return decrypted_data.decode()
    
    def _check_alert_threshold(self, level: str) -> bool:
        """Check if alert threshold has been reached."""
        current_time = datetime.now()
        
        # Reset counter if window has passed
        if (current_time - self.last_alert_time).seconds >= self.alert_window:
            self.error_count = 0
            self.last_alert_time = current_time
        
        if level in self.alert_thresholds:
            if level == "CRITICAL":
                return True
            
            self.error_count += 1
            if self.error_count >= self.alert_thresholds[level]:
                self.error_count = 0
                return True
        
        return False
    
    def _send_alert(self, level: str, message: str):
        """Send alert when threshold is reached."""
        alert_message = f"ALERT: {level} threshold reached - {message}"
        # In practice, you might want to send this to an external system
        # like email, Slack, or a monitoring service
        print(alert_message)  # Replace with actual alert mechanism
    
    def log_event(self, 
                  level: str, 
                  message: str, 
                  sensitive_data: Optional[Dict[str, Any]] = None):
        """
        Log an event with optional sensitive data encryption.
        
        Args:
            level: Log level (INFO, WARNING, ERROR, CRITICAL)
            message: Main log message
            sensitive_data: Dictionary of sensitive data to be encrypted
        """
        try:
            # Handle sensitive data
            if sensitive_data:
                encrypted_data = {
                    key: self._encrypt_sensitive_data(str(value))
                    for key, value in sensitive_data.items()
                }
                message = f"{message} [ENCRYPTED: {json.dumps(encrypted_data)}]"
            
            # Log the message
            log_level = getattr(logging, level.upper())
            self.logger.log(log_level, message)
            
            # Check alert threshold
            if self._check_alert_threshold(level.upper()):
                self._send_alert(level, message)
                
        except Exception as e:
            print(f"Logging failed: {str(e)}")
            
    def analyze_logs(self, log_file: str) -> Dict[str, Any]:
        """
        Analyze log file and return statistics.
        
        Returns dictionary with:
        - event counts by level
        - error frequency
        - most common event types
        """
        stats = {
            "level_counts": {},
            "error_frequency": 0,
            "common_events": {}
        }
        
        try:
            with open(log_file, 'r') as f:
                for line in f:
                    if ' - ' in line:
                        level = line.split(' - ')[1]
                        stats["level_counts"][level] = stats["level_counts"].get(level, 0) + 1
                        
                        if level in ["ERROR", "CRITICAL"]:
                            stats["error_frequency"] += 1
                            
                        event_type = line.split(' - ')[-1].split()[0]
                        stats["common_events"][event_type] = stats["common_events"].get(event_type, 0) + 1
                        
            # Calculate error frequency per hour
            total_time = (datetime.now() - self.last_alert_time).total_seconds() / 3600
            if total_time > 0:
                stats["error_frequency"] = stats["error_frequency"] / total_time
                
        except Exception as e:
            print(f"Log analysis failed: {str(e)}")
            
        return stats

# Beispielverwendung:
if __name__ == "__main__":
    # Initialize the secure logger
    logger = SecureLogger(
        log_file="system_events.log",
        max_bytes=1_000_000,  # 1MB
        backup_count=3
    )
    
    # Log some events
    logger.log_event(
        "INFO",
        "User login attempt",
        sensitive_data={"username": "john_doe", "ip_address": "192.168.1.1"}
    )
    
    logger.log_event(
        "ERROR",
        "Database connection failed",
        sensitive_data={"connection_string": "sensitive_connection_details"}
    )
    
    # Analyze logs
    stats = logger.analyze_logs("system_events.log")
    print("Log Analysis Results:", json.dumps(stats, indent=2))