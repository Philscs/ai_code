from typing import Any, Dict, Type, get_type_hints
import json
import inspect
from dataclasses import dataclass
import pickle

class ValidationError(Exception):
    """Custom exception for validation errors."""
    pass

class SecureAttributeMetaclass(type):
    """
    Eine Metaclass für sichere Attributvalidierung mit Typüberprüfung 
    und Schutz vor Code-Injection.
    """
    
    def __new__(mcs, name: str, bases: tuple, namespace: dict) -> Type:
        # Sammle alle Typ-Annotations
        type_hints = get_type_hints(namespace.get('__annotations__', {}))
        
        # Validiere existierende Klassenattribute
        for attr_name, attr_value in namespace.items():
            if attr_name in type_hints:
                mcs._validate_attribute(attr_name, attr_value, type_hints[attr_name])
        
        # Erstelle Property-Wrapper für alle typisierten Attribute
        for attr_name, attr_type in type_hints.items():
            if attr_name not in namespace:
                namespace[attr_name] = None
            
            # Erstelle Property mit Getter und Setter
            namespace[attr_name] = mcs._create_validated_property(
                attr_name, 
                attr_type,
                namespace.get(attr_name)
            )
            
        # Implementiere sichere Serialisierung
        def to_json(self) -> str:
            """Sichere JSON-Serialisierung der Instanz."""
            data = {
                attr: getattr(self, attr)
                for attr in type_hints
                if hasattr(self, attr)
            }
            return json.dumps(data, default=str)
        
        namespace['to_json'] = to_json
        
        # Verhindere unsichere Pickle-Serialisierung
        def __getstate__(self) -> Dict:
            """Sicheres State Management für Serialisierung."""
            state = self.__dict__.copy()
            # Entferne potentiell unsichere Attribute
            for key in list(state.keys()):
                if key.startswith('_') or callable(state[key]):
                    del state[key]
            return state
        
        namespace['__getstate__'] = __getstate__
        
        # Implementiere Custom __setattr__ für zusätzliche Sicherheit
        def __setattr__(self, name: str, value: Any) -> None:
            """
            Überschreibe __setattr__ um dynamische Attributzuweisungen zu validieren.
            """
            # Überprüfe ob das Attribut erlaubt ist
            if name not in type_hints and not name.startswith('_'):
                raise ValidationError(
                    f"Dynamische Zuweisung von '{name}' ist nicht erlaubt"
                )
            
            # Validiere den Wert wenn ein Typ-Hint existiert
            if name in type_hints:
                mcs._validate_attribute(name, value, type_hints[name])
            
            # Setze das Attribut
            super(type(self), self).__setattr__(name, value)
            
        namespace['__setattr__'] = __setattr__
        
        return super().__new__(mcs, name, bases, namespace)
    
    @staticmethod
    def _validate_attribute(name: str, value: Any, expected_type: Type) -> None:
        """
        Validiere ein Attribut gegen seinen erwarteten Typ und führe
        zusätzliche Sicherheitschecks durch.
        """
        # Überprüfe None-Werte
        if value is None:
            if getattr(expected_type, '__origin__', None) is not type(None):
                raise ValidationError(f"Attribut '{name}' darf nicht None sein")
            return
            
        # Überprüfe den Typ
        if not isinstance(value, expected_type):
            raise ValidationError(
                f"Attribut '{name}' muss vom Typ {expected_type} sein, "
                f"ist aber {type(value)}"
            )
            
        # Zusätzliche Sicherheitschecks für Strings
        if isinstance(value, str):
            # Überprüfe auf potenziell gefährliche Strings
            dangerous_patterns = ['__', 'eval', 'exec', 'import']
            if any(pattern in value for pattern in dangerous_patterns):
                raise ValidationError(
                    f"Attribut '{name}' enthält potenziell unsicheren String"
                )
    
    @staticmethod
    def _create_validated_property(
        attr_name: str,
        attr_type: Type,
        default_value: Any = None
    ) -> property:
        """
        Erstelle eine Property mit Validierung für ein typisiertes Attribut.
        """
        storage_name = f'_{attr_name}'
        
        def getter(instance: Any) -> Any:
            return getattr(instance, storage_name, default_value)
            
        def setter(instance: Any, value: Any) -> None:
            SecureAttributeMetaclass._validate_attribute(
                attr_name, value, attr_type
            )
            setattr(instance, storage_name, value)
            
        return property(getter, setter)

# Beispielnutzung
@dataclass
class Person(metaclass=SecureAttributeMetaclass):
    name: str
    age: int
    email: str = None

# Beispiel für eine Klasse mit Invarianten
class BankAccount(metaclass=SecureAttributeMetaclass):
    balance: float
    account_number: str
    
    def __init__(self, account_number: str, initial_balance: float = 0.0):
        self.account_number = account_number
        self.balance = initial_balance
    
    def withdraw(self, amount: float) -> None:
        if amount > self.balance:
            raise ValueError("Nicht genügend Guthaben")
        self.balance -= amount
    
    def deposit(self, amount: float) -> None:
        if amount <= 0:
            raise ValueError("Einzahlungsbetrag muss positiv sein")
        self.balance += amount