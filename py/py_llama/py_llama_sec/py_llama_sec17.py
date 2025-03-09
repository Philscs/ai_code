import inspect
from typing import TypeVar, Generic, Any

# Definiere eine allgemeine Variablenklasse T, die als Type-Hint dient
T = TypeVar('T')

class ValidatedMeta(type):
    """
    Custom Metaclass für die automatische Validierung von Klassenattributen.
    """

    # Speichere alle registrierten Invarianten in einem Dictionary
    _invariants = {}

    def __new__(cls, name, bases, dct):
        # Registriere neue Invarianten
        cls._register_invariant(name)

        # Erstelle eine Instanz des Metaklasses
        instance = super().__new__(cls, name, bases, dct)

        # Wenn das Objekt ein Typ ist und Invarianten implementiert
        if inspect.isclass(instance):
            # Überprüfe alle Attribute mit dem Typ 'invariant'
            for attr_name in dir(instance):
                attr_value = getattr(instance, attr_name)
                if attr_value.startswith('invariant_'):
                    # Registriere die Invariant-Implementierung
                    cls._register_invariant(attr_name)

        return instance

    def _register_invariant(self, name: str):
        """
        Registriere eine neue Invariant-Implementierung.
        """
        self._invariants[name] = inspect.getsource(instance.__class__.__name__)

    @classmethod
    def validate(cls, obj: Any) -> None:
        """
        Überprüfe die Validität eines Objekts.
        """
        # Überprüfe alle registrierten Invarianten
        for invariant_name in cls._invariants:
            if hasattr(obj, invariant_name):
                # Wenn das Attribut nicht vorhanden ist, wirft eine Exeption
                if not callable(getattr(obj, invariant_name)):
                    raise ValueError(f"Invariant '{invariant_name}' fehlt")
                # Wenn die Invariant-Implementierung gefunden wird, überprüfe ihre Gültigkeit
                source = cls._invariants[invariant_name]
                try:
                    eval(source)
                except Exception as e:
                    raise ValueError(f"Invalidation vonInvariant '{invariant_name}': {str(e)}")

# Verwende den Custom Metaclass für eine Klassenvalidierung
class ValidatedClass(metaclass=ValidatedMeta):
    invariant_a = 1
    invariant_b: int

    def __init__(self, a: int, b: int) -> None:
        # Invariant a überprüfen
        self.invariant_a = a
        # Invariant b überprüfen
        if b != 3:
            raise ValueError("Invariant b ist falsch")

class InvalidClass(metaclass=ValidatedMeta):
    invariant_a = 1

# Verwende die validate-Method, um die Validität zu überprüfen
def main():
    valid_obj = ValidatedClass(1, 3)
    try:
        ValidatedMeta.validate(valid_obj) # Überprüfung der gültigkeit von Invariant a und b
    except ValueError as e:
        print(f"Gültigkeit von Invarianten nicht überprüft: {e}")
    
    invalid_obj = InvalidClass()
    try:
        ValidatedMeta.validate(invalid_obj)
    except ValueError as e:
        print(f"Gültigkeit von Invarianten nicht überprüft: {e}")

if __name__ == "__main__":
    main()
