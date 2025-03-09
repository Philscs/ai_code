import logging
import re
from collections import defaultdict

class SafeLoggingHandler(logging.Handler):
    def __init__(self, targets):
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

            # Filter logs (Example: send only WARN and ERROR to the console)
            if record.levelno >= logging.WARN:
                self._send_to_target('console', record)

            # Aggregate logs for further processing
            self._aggregate_logs(record)

            # Forward logs to file logging
            self._send_to_target('file', record)

        except Exception as e:
            print(f"Fehler im Logging-Handler: {e}")

    def _sanitize_message(self, message):
        """Mask sensitive data and prevent log injection."""
        # Mask sensitive data
        sanitized_message = self.sensitive_data_pattern.sub('<MASKIERT>', message)

        # Prevent log injection
        sanitized_message = sanitized_message.replace('\n', ' ').replace('\r', ' ')

        return sanitized_message

    def _send_to_target(self, target_name, record):
        """Send log events to the specified target."""
        if target_name in self.targets:
            target_handler = self.targets[target_name]
            target_handler.emit(record)

    def _aggregate_logs(self, record):
        """Aggregate logs for further processing."""
        self.aggregated_logs[record.levelname].append(record.getMessage())

# Example Setup
if __name__ == "__main__":
    # Create target handlers
    console_handler = logging.StreamHandler()
    file_handler = logging.FileHandler("app.log")

    # Configure logger
    logger = logging.getLogger("CustomLogger")
    logger.setLevel(logging.DEBUG)

    # Add custom handler
    custom_handler = SafeLoggingHandler(targets={
        'console': console_handler,
        'file': file_handler
    })
    logger.addHandler(custom_handler)

    # Generate log events
    logger.info("This is an info log.")
    logger.warning("Warning! Credit card number: 1234-5678-9012-3456")
    logger.error("Error! User email: test@example.com")
