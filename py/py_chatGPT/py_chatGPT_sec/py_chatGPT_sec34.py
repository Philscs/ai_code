import logging
import traceback
import json
from datetime import datetime

class SensitiveInfoFilter(logging.Filter):
    def filter(self, record):
        if hasattr(record, 'message') and isinstance(record.message, str):
            # Beispiel: Filterung sensibler Informationen
            record.message = record.message.replace('SECRET', '***')
        return True

def clean_stack_trace(stack_trace):
    """
    Bereinigt die Stack Trace, indem spezifische Systeminformationen entfernt werden.
    """
    clean_lines = []
    for line in stack_trace.splitlines():
        if "sensitive_path" not in line:  # Beispiel für Filterung von Pfaden
            clean_lines.append(line)
    return "\n".join(clean_lines)

def create_structured_error_log(exc_type, exc_value, tb):
    """
    Erstellt ein strukturiertes Fehlerprotokoll.
    """
    stack_trace = traceback.format_exception(exc_type, exc_value, tb)
    cleaned_trace = clean_stack_trace("".join(stack_trace))

    error_log = {
        "timestamp": datetime.utcnow().isoformat(),
        "error_type": exc_type.__name__,
        "error_message": str(exc_value).replace('SECRET', '***'),  # Filtere sensible Daten
        "stack_trace": cleaned_trace
    }
    return error_log

def custom_exception_handler(exc_type, exc_value, tb):
    """
    Benutzdefinierter Exception-Handler, der die Fehler behandelt.
    """
    # Protokollierung vorbereiten
    logger = logging.getLogger("CustomExceptionHandler")
    logger.setLevel(logging.ERROR)

    # Filter hinzufügen
    sensitive_filter = SensitiveInfoFilter()
    logger.addFilter(sensitive_filter)

    # Ausgabeformat
    handler = logging.StreamHandler()
    handler.setFormatter(logging.Formatter('%(asctime)s - %(levelname)s - %(message)s'))
    logger.addHandler(handler)

    # Fehlerprotokoll erstellen
    structured_log = create_structured_error_log(exc_type, exc_value, tb)

    # Fehlerprotokoll als JSON loggen
    logger.error(json.dumps(structured_log, indent=4))

# Beispielhafter Einsatz des Handlers
if __name__ == "__main__":
    import sys

    sys.excepthook = custom_exception_handler

    # Beispiel für einen Fehler mit sensiblen Informationen
    SECRET = "12345"
    raise ValueError(f"Ein Fehler mit sensitiven Daten: {SECRET}")
