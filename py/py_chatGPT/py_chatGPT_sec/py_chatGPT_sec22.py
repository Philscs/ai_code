import threading
import functools
import time
from hashlib import sha256

class ThreadSafeMethodCache:
    def __init__(self, ttl=60):
        """
        Initialisiert den Cache mit einer Time-to-Live (TTL).

        :param ttl: Zeit in Sekunden, nach der Einträge aus dem Cache entfernt werden (Standard: 60).
        """
        self.cache = {}
        self.ttl = ttl
        self.lock = threading.Lock()

    def _make_key(self, args, kwargs):
        """
        Erzeugt einen eindeutigen Schlüssel basierend auf den Argumenten der Methode.

        :param args: Argumente der Methode
        :param kwargs: Keyword-Argumente der Methode
        :return: Hash-Wert als Schlüssel
        """
        key = sha256(
            (str(args) + str(kwargs)).encode('utf-8')
        ).hexdigest()
        return key

    def get(self, key):
        """
        Ruft einen Wert aus dem Cache ab, wenn er noch gültig ist.

        :param key: Schlüssel des Cache-Eintrags
        :return: Der gecachte Wert oder None, falls der Schlüssel nicht existiert oder abgelaufen ist
        """
        with self.lock:
            if key in self.cache:
                value, timestamp = self.cache[key]
                if time.time() - timestamp < self.ttl:
                    return value
                else:
                    # Eintrag ist abgelaufen
                    del self.cache[key]
            return None

    def set(self, key, value):
        """
        Fügt einen Wert zum Cache hinzu.

        :param key: Schlüssel des Cache-Eintrags
        :param value: Zu speichernder Wert
        """
        with self.lock:
            self.cache[key] = (value, time.time())

    def invalidate(self, key):
        """
        Entfernt einen spezifischen Schlüssel aus dem Cache.

        :param key: Schlüssel des Cache-Eintrags
        """
        with self.lock:
            if key in self.cache:
                del self.cache[key]

    def clear(self):
        """
        Entfernt alle Einträge aus dem Cache.
        """
        with self.lock:
            self.cache.clear()


def method_cache(ttl=60):
    """
    Custom Decorator für Method-Caching.

    :param ttl: Zeit in Sekunden, nach der Cache-Einträge ablaufen (Standard: 60).
    :return: Dekorierte Methode
    """
    cache = ThreadSafeMethodCache(ttl=ttl)

    def decorator(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            # Erzeuge den Cache-Schlüssel basierend auf den Argumenten
            key = cache._make_key(args, kwargs)
            
            # Prüfe, ob der Wert im Cache existiert
            result = cache.get(key)
            if result is not None:
                return result

            # Falls nicht im Cache, führe die Methode aus
            result = func(*args, **kwargs)

            # Speichere das Ergebnis im Cache (falls nicht sensitiv)
            cache.set(key, result)
            return result

        def invalidate(*args, **kwargs):
            """
            Invalidiert einen spezifischen Cache-Eintrag basierend auf den Argumenten.

            :param args: Argumente der Methode
            :param kwargs: Keyword-Argumente der Methode
            """
            key = cache._make_key(args, kwargs)
            cache.invalidate(key)

        def clear():
            """
            Löscht alle Cache-Einträge.
            """
            cache.clear()

        # Zusätzliche Methoden für den Zugriff auf Cache-Management
        wrapper.invalidate = invalidate
        wrapper.clear_cache = clear

        return wrapper

    return decorator

# Beispielnutzung
if __name__ == "__main__":
    @method_cache(ttl=30)
    def slow_function(x):
        time.sleep(2)  # Simuliert eine langsame Funktion
        return x * x

    print(slow_function(4))  # Berechnung
    print(slow_function(4))  # Cache

    # Invalidate Cache
    slow_function.invalidate(4)
    print(slow_function(4))  # Neue Berechnung
