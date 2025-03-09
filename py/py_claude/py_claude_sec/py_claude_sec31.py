from typing import Any, Callable, Dict, List, Optional, Type, Union
from abc import ABC, abstractmethod
from dataclasses import dataclass
import re
from enum import Enum

class ValidationError(Exception):
    """Basisklasse für Validierungsfehler"""
    pass

class Language(Enum):
    DE = "de"
    EN = "en"

@dataclass
class ValidationResult:
    is_valid: bool
    errors: List[str]

class Constraint(ABC):
    """Abstrakte Basisklasse für Validierungsregeln"""
    
    def __init__(self, error_messages: Dict[Language, str]):
        self.error_messages = error_messages
    
    @abstractmethod
    def validate(self, value: Any, language: Language) -> Optional[str]:
        """Führt die Validierung durch und gibt ggf. eine Fehlermeldung zurück"""
        pass

class TypeConstraint(Constraint):
    """Validiert den Typ eines Wertes"""
    
    def __init__(self, expected_type: Type, error_messages: Dict[Language, str]):
        super().__init__(error_messages)
        self.expected_type = expected_type
    
    def validate(self, value: Any, language: Language) -> Optional[str]:
        if not isinstance(value, self.expected_type):
            return self.error_messages.get(language, self.error_messages[Language.EN])
        return None

class RangeConstraint(Constraint):
    """Validiert, ob ein Wert innerhalb eines Bereichs liegt"""
    
    def __init__(self, min_value: Union[int, float], max_value: Union[int, float], 
                 error_messages: Dict[Language, str]):
        super().__init__(error_messages)
        self.min_value = min_value
        self.max_value = max_value
    
    def validate(self, value: Any, language: Language) -> Optional[str]:
        if not isinstance(value, (int, float)) or value < self.min_value or value > self.max_value:
            return self.error_messages.get(language, self.error_messages[Language.EN])
        return None

class RegexConstraint(Constraint):
    """Validiert einen String gegen einen regulären Ausdruck"""
    
    def __init__(self, pattern: str, error_messages: Dict[Language, str]):
        super().__init__(error_messages)
        # Sicherheitscheck für potenziell gefährliche Regex-Muster
        if self._is_unsafe_pattern(pattern):
            raise ValueError("Unsicheres Regex-Muster erkannt")
        self.pattern = re.compile(pattern)
    
    def _is_unsafe_pattern(self, pattern: str) -> bool:
        """Überprüft auf potenziell gefährliche Regex-Muster"""
        # Beispiel für unsichere Muster:
        unsafe_patterns = [
            r'(a+){10,}',  # Potenziell exponentieller Backtracking
            r'([a-zA-Z]+)*\d',  # Nested quantifiers
        ]
        return any(unsafe in pattern for unsafe in unsafe_patterns)
    
    def validate(self, value: Any, language: Language) -> Optional[str]:
        if not isinstance(value, str) or not self.pattern.match(value):
            return self.error_messages.get(language, self.error_messages[Language.EN])
        return None

class CustomConstraint(Constraint):
    """Ermöglicht benutzerdefinierte Validierungsfunktionen"""
    
    def __init__(self, validation_func: Callable[[Any], bool], error_messages: Dict[Language, str]):
        super().__init__(error_messages)
        self.validation_func = validation_func
    
    def validate(self, value: Any, language: Language) -> Optional[str]:
        try:
            if not self.validation_func(value):
                return self.error_messages.get(language, self.error_messages[Language.EN])
        except Exception:
            return self.error_messages.get(language, self.error_messages[Language.EN])
        return None

class DataValidator:
    """Hauptklasse für die Validierung von Daten"""
    
    def __init__(self, language: Language = Language.EN):
        self.constraints: Dict[str, List[Constraint]] = {}
        self.language = language
    
    def add_field(self, field_name: str, constraints: List[Constraint]) -> None:
        """Fügt Validierungsregeln für ein Feld hinzu"""
        self.constraints[field_name] = constraints
    
    def validate(self, data: Dict[str, Any]) -> ValidationResult:
        """Führt die Validierung für alle Felder durch"""
        errors = []
        
        for field_name, field_constraints in self.constraints.items():
            if field_name not in data:
                errors.append(f"Feld '{field_name}' fehlt")
                continue
                
            value = data[field_name]
            for constraint in field_constraints:
                error = constraint.validate(value, self.language)
                if error:
                    errors.append(f"{field_name}: {error}")
        
        return ValidationResult(len(errors) == 0, errors)

# Beispiel für die Verwendung
def example_usage():
    # Erstellen eines Validators
    validator = DataValidator(Language.DE)
    
    # Definieren von Fehlermeldungen
    type_errors = {
        Language.DE: "Ungültiger Typ",
        Language.EN: "Invalid type"
    }
    range_errors = {
        Language.DE: "Wert außerhalb des gültigen Bereichs",
        Language.EN: "Value out of range"
    }
    email_errors = {
        Language.DE: "Ungültige E-Mail-Adresse",
        Language.EN: "Invalid email address"
    }
    
    # Hinzufügen von Validierungsregeln
    validator.add_field("age", [
        TypeConstraint(int, type_errors),
        RangeConstraint(0, 150, range_errors)
    ])
    
    validator.add_field("email", [
        TypeConstraint(str, type_errors),
        RegexConstraint(r'^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$', email_errors)
    ])
    
    # Beispieldaten
    data = {
        "age": 25,
        "email": "test@example.com"
    }
    
    # Durchführen der Validierung
    result = validator.validate(data)
    return result

if __name__ == "__main__":
    result = example_usage()
    print(f"Validierung erfolgreich: {result.is_valid}")
    if not result.is_valid:
        print("Fehler:")
        for error in result.errors:
            print(f"- {error}")