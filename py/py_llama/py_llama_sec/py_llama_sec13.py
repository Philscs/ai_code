import logging

# Custom Logging Handler
class LogHandler(logging.Handler):
    def __init__(self, filter_level=10, aggregate=True, target='stdout'):
        super().__init__()
        self.filter_level = filter_level
        self.aggregate = aggregate
        self.target = target

    def emit(self, record):
        if record.levelno <= self.filter_level:
            # Log-Ereignis filtern
            if hasattr(record, 'message') and not self.is_sensitive_data(record['message']):
                message = record.get('message', '')
            else:
                message = f"SENSITIVE DATA: {record.get('original_message', '')}"

            # Log-Ereignis aggregieren (optional)
            if self.aggregate:
                for other in self.handlers:
                    try:
                        other.handle(record)
                    except TypeError:
                        pass

            # Log-Ereignis an Ziel weiterleiten
            if hasattr(self, 'target'):
                target = getattr(self, self.target)
                target(message)
            else:
                print(message)


    @staticmethod
    def is_sensitive_data(data):
        # Hier werden sensible Daten wie z.B. Login-Informationen maskiert
        sensitive_keywords = ['passwort', 'token']
        for keyword in sensitive_keywords:
            if keyword.lower() in str(data).lower():
                return True
        return False


# Logging Anwendung
if __name__ == "__main__":
    # Erstelle einen Log-Formatter, der die Daten formatiert
    formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')

    # Erstelle den Log Handler mit den gewünschten Filtern und Ziel
    handler = LogHandler(filter_level=logging.INFO, aggregate=True, target='stdout')
    handler.setLevel(logging.INFO)

    # Erstelle einen Logger
    logger = logging.getLogger('custom_logger')
    logger.addHandler(handler)

    # Teste den Logger
    for i in range(10):
        logger.info(f"This is a test message {i}")