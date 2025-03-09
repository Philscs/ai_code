import logging
import logging.handlers
import os
from cryptography.fernet import Fernet
import smtplib
from email.message import EmailMessage
import json

def load_config():
    """Lädt die Konfigurationsdatei."""
    with open("config.json", "r") as file:
        return json.load(file)

# Verschlüsselungsschlüssel erzeugen oder laden
key_file = "encryption.key"
if not os.path.exists(key_file):
    with open(key_file, "wb") as f:
        f.write(Fernet.generate_key())

with open(key_file, "rb") as f:
    ENCRYPTION_KEY = f.read()

encryptor = Fernet(ENCRYPTION_KEY)

class SecureLogFormatter(logging.Formatter):
    """Formatierer zur Verschleierung sensibler Informationen."""

    def format(self, record):
        record.msg = self._mask_sensitive_info(record.msg)
        return super().format(record)

    def _mask_sensitive_info(self, message):
        sensitive_keywords = ["password", "secret", "api_key"]
        for keyword in sensitive_keywords:
            if keyword in message:
                message = message.replace(keyword, "***")
        return message

class LogAnalyzer:
    """Klasse zur Analyse von Logs und Alarmierung."""

    def __init__(self, config):
        self.config = config

    def analyze_log(self, log_message):
        for alert in self.config["alerts"]:
            if alert["keyword"] in log_message:
                self.send_alert(alert["message"])

    def send_alert(self, alert_message):
        """Versendet eine Alarmierung per E-Mail."""
        if not self.config.get("email_alerts_enabled"):
            return

        msg = EmailMessage()
        msg.set_content(alert_message)
        msg["Subject"] = "System Alert"
        msg["From"] = self.config["email_sender"]
        msg["To"] = self.config["email_recipient"]

        with smtplib.SMTP(self.config["smtp_server"], self.config["smtp_port"]) as server:
            server.starttls()
            server.login(self.config["email_sender"], self.config["email_password"])
            server.send_message(msg)

def setup_logger(config):
    """Richtet den Logger ein."""
    logger = logging.getLogger("SecureLogger")
    logger.setLevel(logging.DEBUG)

    log_formatter = SecureLogFormatter(
        "%(asctime)s - %(levelname)s - %(message)s"
    )

    # Datei-Handler mit Rotation
    log_handler = logging.handlers.RotatingFileHandler(
        config["log_file"], maxBytes=config["log_max_bytes"], backupCount=config["log_backup_count"]
    )
    log_handler.setFormatter(log_formatter)

    logger.addHandler(log_handler)
    return logger

# Beispiel für die Konfigurationsdatei
example_config = {
    "log_file": "system.log",
    "log_max_bytes": 1024 * 1024,  # 1 MB
    "log_backup_count": 5,
    "alerts": [
        {"keyword": "ERROR", "message": "Ein Fehler ist im System aufgetreten!"},
        {"keyword": "CRITICAL", "message": "Kritischer Fehler erkannt!"}
    ],
    "email_alerts_enabled": True,
    "email_sender": "sender@example.com",
    "email_recipient": "recipient@example.com",
    "smtp_server": "smtp.example.com",
    "smtp_port": 587,
    "email_password": "example_password"
}

# Speichern der Beispielkonfiguration (nur bei Bedarf)
# with open("config.json", "w") as f:
#     json.dump(example_config, f, indent=4)

# System initialisieren
config = load_config()
logger = setup_logger(config)
analyzer = LogAnalyzer(config)

# Beispiel-Log-Nachrichten
logger.info("System gestartet.")
logger.error("Ein unerwarteter Fehler ist aufgetreten.")
logger.critical("Datenbankverbindung fehlgeschlagen!")

# Logs analysieren
with open(config["log_file"], "r") as log_file:
    for line in log_file:
        analyzer.analyze_log(line)
