from typing import TYPE_CHECKING, get_type_hints
import functools

if TYPE_CHECKING:
    from typing import Callable

def type_check(func: Callable) -> Callable:
    """Typsicherheitsprüfer für eine Funktion.

    Die Funktion wird nur dann ausgewertet, wenn der Anrufer den gleichen Typ hat wie die 
Funktion.

    Args:
        func (Callable): Die Funktion, die überprüft werden soll.

    Returns:
        Callable: Eine Funktion, die den gleichen Typ wie die ursprüngliche Funktion hat.
    """

    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        # Erstelle einen Typensatz für die Argumente
        arg_types = get_type_hints(type(args))
        
        # Überprüfe den Typ der Argumente gegenüber dem Typ der Funktion
        if not all(arg_type == type(func.__annotations__['return']) for arg_type in 
arg_types.values()):
            raise TypeError(f"Typ von '{func.__name__}' ist nicht kompatibel mit den 
Argumenten.")

        return func(*args, **kwargs)

    # Ersetze die Funktionsadresse für einen Wrapper
    wrapper.__wrapped__ = func
    return wrapper

# Beispiel für eine typsichere Funktion
@type_check
def add(a: int, b: int) -> int:
    """Funktion zur Berechnung der Summe zweier Zahlen."""
    return a + b

print(add(2, 3))  # Ausgabe: 5

try:
    print(add('a', 'b'))  # Ausnahme
except TypeError as e:
    print(e)

# Typsicherheitsprüfung für ein spezifisches Argument
@type_check
def greet(name: str) -> None:
    """Funktion zum Gruß einer Person."""
    print(f"Hello, {name}!")

greet('Alice')  # Ausgabe: Hello, Alice!

try:
    greet(123)  # Ausnahme
except TypeError as e:
    print(e)
