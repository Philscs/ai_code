import logging
import re
from collections import defaultdict

class SafeLoggingHandler(logging.Handler):
    def __init__(self, targets):
        """
        Custom Logging Handler, der Log-Ereignisse filtert, aggregiert
        und an verschiedene Ziele weiterleitet.

        :param targets: Dictionary mit Ziel-Handlern, z. B. {'console': console_handler, 'file': file_handler}
        """
        super().__init__()
        self.targets = targets
        self.aggregated_logs = defaultdict(list)
        self.sensitive_data_pattern = re.compile(r'(\b(\d{4}-?\d{4}-?\d{4}-?\d{4})\b|\b\w+@\w+\.\w+\b)')

    def emit(self, record):
        try:
            # Sanitize Log Message
            sanitized_message = self._sanitize_message(record.getMessage())
            record.msg = sanitized_message
            record.args = ()

            # Filter logs (Beispiel: nur WARN und ERROR an die Konsole senden)
            if record.levelno >= logging.WARN:
                self._send_to_target('console', record)

            # Aggregieren für andere Verarbeitung
            self.aggregated_logs[record.levelname].append(sanitized_message)

            # Weiterleitung an Datei-Logging
            self._send_to_target('file', record)

        except Exception as e:
            print(f"Fehler im Logging-Handler: {e}")

    def _sanitize_message(self, message):
        """Maskiere sensitive Daten und verhindere Log-Injection."""
        # Maskiere sensitive Daten
        sanitized_message = self.sensitive_data_pattern.sub('<MASKIERT>', message)

        # Verhindere Log-Injection
        sanitized_message = sanitized_message.replace('\n', ' ').replace('\r', ' ')

        return sanitized_message

    def _send_to_target(self, target_name, record):
        """Sende Log-Ereignisse an das angegebene Ziel."""
        if target_name in self.targets:
            target_handler = self.targets[target_name]
            target_handler.emit(record)

# Beispiel-Setup
if __name__ == "__main__":
    # Ziel-Handler erstellen
    console_handler = logging.StreamHandler()
    file_handler = logging.FileHandler("app.log")

    # Logger konfigurieren
    logger = logging.getLogger("CustomLogger")
    logger.setLevel(logging.DEBUG)

    # Custom Handler hinzufügen
    custom_handler = SafeLoggingHandler(targets={
        'console': console_handler,
        'file': file_handler
    })
    logger.addHandler(custom_handler)

    # Log-Ereignisse erzeugen
    logger.info("Dies ist ein Info-Log.")
    logger.warning("Warnung! Kreditkartennummer: 1234-5678-9012-3456")
    logger.error("Fehler! Benutzer*in E-Mail: test@example.com")
