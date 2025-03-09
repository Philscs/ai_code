import logging
from traceback import format_exc as format_traceback
from datetime import datetime

# Definiere eine Klasse für unsere Exception
class GesetzlichUnzulässigeAktion(Exception):
    pass


def log_error(exception, level='ERROR'):
    """
    Loggt einen Fehler mit einer strukturierten Fehlerprotokoll.

    Args:
        exception (Exception): Die Ausnahme, die aufgezeichnet werden soll.
        level (str): Der Eingangslevel der Aufzeichnung. Standardmäßig ist dies 'ERROR'.
    """
    # Filtere sensible Informationen
    message = str(exception).replace('\n', '').replace('\t', '')

    # Formatiere den Stack-Trace, um keine internen Systeminformationen zu zeigen
    formatted_traceback = format_traceback(exception)
    formatted_traceback = ''.join(line.split()[1:] for line in formatted_traceback.splitlines())

    # Erstelle ein strukturiertes Fehlerprotokoll
    error_record = {
        'Date': datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        'Time': datetime.now().strftime("%H:%M:%S"),
        'Severity': level.upper(),
        'Message': message,
        'Stack Trace': formatted_traceback
    }

    # Logge das Fehlerprotokoll
    logging.log(level, **error_record)


# Definiere eine umfassende Exception-Klasse
class GesetzlicheUnerwarteteAktion(Exception):
    def __init__(self, message, code):
        self.message = message
        self.code = code
        super().__init__()

    @property
    def formatted_message(self):
        return f"Code {self.code}: {self.message}"

    @formatted_message.setter
    def formatted_message(self, value):
        if not isinstance(value, str):
            raise TypeError("message should be a string")

# Beispiele für die Verwendung der umfassenden Exception-Klasse
try:
    # Code, der eine GesetzlichUnzulässigeAktion auslöst
    raise GesetzlicheUnerwarteteAktion('Ein ungültiger Code wurde eingegeben', 404)
except GesetzlicheUnerwarteteAktion as exception:
    log_error(exception)

try:
    # Code, der eine GesetzlichUnzulässigeAktion auslöst
    raise GesetzlicheUnzulässigeAktion('Ein ungültiger Code wurde eingegeben', 404)
except GesetzlicheUnerwarteteAktion as exception:
    log_error(exception)
