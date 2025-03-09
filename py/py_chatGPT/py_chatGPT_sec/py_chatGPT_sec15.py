class SafeResourceHandler:
    def __init__(self, resource):
        """
        Initialisiert den Context Manager mit der Ressource.
        :param resource: Eine generische Ressource (z. B. Datei, Netzwerkverbindung, etc.)
        """
        self.resource = resource

    def __enter__(self):
        """
        Initialisierungscode, der beim Eintritt in den Kontext ausgeführt wird.
        Gibt die Ressource zurück, damit sie innerhalb des Kontextes genutzt werden kann.
        """
        print("Resource initialized.")
        return self.resource

    def __exit__(self, exc_type, exc_value, traceback):
        """
        Cleanup-Operationen, die beim Verlassen des Kontextes ausgeführt werden.
        :param exc_type: Der Typ der Ausnahme, falls eine auftritt.
        :param exc_value: Der Wert der Ausnahme.
        :param traceback: Der Traceback der Ausnahme.
        :return: True, wenn die Ausnahme behandelt wurde, ansonsten False.
        """
        try:
            # Cleanup-Operationen sicherstellen
            print("Cleaning up the resource.")
            if self.resource:
                self.resource.close()
        except Exception as cleanup_error:
            print(f"Error during cleanup: {cleanup_error}")
        finally:
            # Sicherstellen, dass sensitive Daten bereinigt werden
            self.resource = None
            print("Resource is fully cleaned up.")

        # Ausnahmebehandlung (falls erforderlich)
        if exc_type:
            print(f"Exception occurred: {exc_type}, {exc_value}")
            return False  # Ausnahme wird nicht unterdrückt
        return True  # Ausnahme wird unterdrückt, falls gewünscht

# Beispiel zur Nutzung des Context Managers
if __name__ == "__main__":
    class MockResource:
        """Mock-Klasse zur Simulation einer Ressource."""
        def close(self):
            print("MockResource closed.")

    try:
        with SafeResourceHandler(MockResource()) as resource:
            print("Using the resource.")
            # Simuliere eine Ausnahme
            raise ValueError("Something went wrong!")
    except ValueError as e:
        print(f"Caught exception: {e}")
