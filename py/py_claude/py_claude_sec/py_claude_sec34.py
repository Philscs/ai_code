import sys
import traceback
import logging
import json
from datetime import datetime
from typing import Dict, Any, Optional
import re

class SensitiveDataFilter:
    """Filter für sensitive Daten wie Passwörter, API-Keys etc."""
    
    SENSITIVE_PATTERNS = [
        r'password\s*[=:]\s*["\']?\w+["\']?',
        r'api[_-]?key\s*[=:]\s*["\']?\w+["\']?',
        r'secret\s*[=:]\s*["\']?\w+["\']?',
        r'token\s*[=:]\s*["\']?\w+["\']?',
        r'credentials\s*[=:]\s*["\']?\w+["\']?'
    ]

    @classmethod
    def filter_sensitive_data(cls, text: str) -> str:
        """Ersetzt sensitive Daten durch [FILTERED]"""
        filtered_text = text
        for pattern in cls.SENSITIVE_PATTERNS:
            filtered_text = re.sub(pattern, '[FILTERED]', filtered_text, flags=re.IGNORECASE)
        return filtered_text

class SecureExceptionHandler:
    """Sicherer Exception Handler mit Filterung und strukturierter Protokollierung"""
    
    def __init__(self, log_file: str = 'error.log'):
        """Initialisiert den Logger"""
        self.logger = self._setup_logger(log_file)
        
    def _setup_logger(self, log_file: str) -> logging.Logger:
        """Konfiguriert den Logger"""
        logger = logging.getLogger('SecureExceptionHandler')
        logger.setLevel(logging.ERROR)
        
        formatter = logging.Formatter(
            '%(asctime)s - %(levelname)s - %(message)s'
        )
        
        file_handler = logging.FileHandler(log_file)
        file_handler.setFormatter(formatter)
        logger.addHandler(file_handler)
        
        return logger
    
    def _format_traceback(self, exc_type: type, exc_value: Exception, exc_traceback: traceback) -> Dict[str, Any]:
        """Formatiert den Traceback in ein sicheres, strukturiertes Format"""
        # Erstelle eine gefilterte Traceback-Liste
        tb_list = []
        for filename, line_num, func, text in traceback.extract_tb(exc_traceback):
            # Entferne sensitive Systempfade
            safe_filename = filename.split('/')[-1]
            
            # Filtere sensitive Daten aus dem Code-Text
            safe_text = SensitiveDataFilter.filter_sensitive_data(str(text))
            
            tb_list.append({
                'file': safe_filename,
                'line': line_num,
                'function': func,
                'code': safe_text
            })
        
        # Erstelle strukturierte Fehlermeldung
        error_info = {
            'timestamp': datetime.now().isoformat(),
            'error_type': exc_type.__name__,
            'error_message': SensitiveDataFilter.filter_sensitive_data(str(exc_value)),
            'traceback': tb_list
        }
        
        return error_info
    
    def handle_exception(self, exc_type: type, exc_value: Exception, exc_traceback: traceback) -> None:
        """Hauptmethode zur Behandlung von Exceptions"""
        try:
            # Formatiere und filtere den Error
            error_info = self._format_traceback(exc_type, exc_value, exc_traceback)
            
            # Logge den gefilterten Error
            self.logger.error(
                f"Exception occurred:\n{json.dumps(error_info, indent=2)}"
            )
            
            # Optionaler öffentlicher Output
            print("Ein Fehler ist aufgetreten. Weitere Details finden Sie im Error-Log.")
            
        except Exception as e:
            # Fallback im Falle eines Fehlers im Exception Handler
            print(f"Fehler im Exception Handler: {str(e)}")
            
    def __enter__(self):
        """Context Manager Entry"""
        self.original_excepthook = sys.excepthook
        sys.excepthook = self.handle_exception
        return self
    
    def __exit__(self, exc_type, exc_value, traceback):
        """Context Manager Exit"""
        sys.excepthook = self.original_excepthook

# Beispiel zur Verwendung:
if __name__ == "__main__":
    # Als Context Manager
    with SecureExceptionHandler(log_file='secure_error.log'):
        # Ihr Code hier
        try:
            # Beispiel-Exception
            password = "geheim123"
            api_key = "sk_live_123456"
            raise ValueError(f"Error with password: {password} and API key: {api_key}")
        except Exception as e:
            raise  # wird vom SecureExceptionHandler abgefangen
    
    # Oder als globaler Exception Handler
    handler = SecureExceptionHandler(log_file='secure_error.log')
    sys.excepthook = handler.handle_exception