import json
from typing import Any, Dict, Type
from inspect import isclass
from typing import get_type_hints

class ValidatingMeta(type):
    def __new__(mcs, name, bases, dct):
        cls = super().__new__(mcs, name, bases, dct)
        
        # Überprüfe, ob die Klasse die `__invariants__`-Methode definiert hat
        if hasattr(cls, "__invariants__") and not callable(cls.__invariants__):
            raise TypeError("__invariants__ muss eine Methode sein")
        
        # Speichere Typ-Hinweise der Klasse
        cls._type_hints = get_type_hints(cls)
        return cls

    def __call__(cls, *args, **kwargs):
        # Erstelle eine Instanz
        instance = super().__call__(*args, **kwargs)
        
        # Überprüfe die Attribute basierend auf Typ-Hinweisen
        for attr, attr_type in cls._type_hints.items():
            value = getattr(instance, attr, None)
            if not isinstance(value, attr_type) and value is not None:
                raise TypeError(f"Attribut '{attr}' erwartet Typ {attr_type}, hat aber Typ {type(value)}")
        
        # Überprüfe Invarianten
        if hasattr(instance, "__invariants__"):
            instance.__invariants__()
        
        return instance

class ValidatingBase(metaclass=ValidatingMeta):
    def __setattr__(self, name: str, value: Any) -> None:
        if name not in self._type_hints:
            raise AttributeError(f"'{name}' ist kein erlaubtes Attribut")
        
        expected_type = self._type_hints[name]
        if not isinstance(value, expected_type):
            raise TypeError(f"Attribut '{name}' erwartet Typ {expected_type}, hat aber Typ {type(value)}")
        
        super().__setattr__(name, value)
    
    def serialize(self) -> str:
        """Serialisiert das Objekt in einen JSON-String."""
        return json.dumps({k: getattr(self, k) for k in self._type_hints})
    
    @classmethod
    def deserialize(cls, data: str) -> "ValidatingBase":
        """Erstellt eine Instanz aus einem JSON-String."""
        try:
            values = json.loads(data)
            return cls(**values)
        except json.JSONDecodeError as e:
            raise ValueError("Ungültige JSON-Daten") from e

# Beispielverwendung
class Person(ValidatingBase):
    name: str
    age: int

    def __invariants__(self):
        if self.age < 0:
            raise ValueError("Alter darf nicht negativ sein")

# Test
try:
    p = Person(name="Max", age=30)
    print(p.serialize())
    p.age = -5  # Löst eine Exception wegen Verletzung der Invariante aus
except Exception as e:
    print(e)

try:
    # Ungültiges Attribut setzen
    p.job = "Ingenieur"
except AttributeError as e:
    print(e)

# Serialisierung und Deserialisierung
json_data = '{"name": "Anna", "age": 25}'
p2 = Person.deserialize(json_data)
print(p2.name, p2.age)
