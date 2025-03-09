class ResourceManager:
    def __init__(self, resource_name, sensitive_data=None):
        """
        Initialisiert den Resource Manager
        
        Args:
            resource_name (str): Name der zu verwaltenden Ressource
            sensitive_data (Any): Optionale sensitive Daten
        """
        self.resource_name = resource_name
        self.sensitive_data = sensitive_data
        self.resource = None

    def __enter__(self):
        """
        Wird beim Betreten des Context Managers aufgerufen
        Initialisiert und öffnet die Ressource
        """
        try:
            print(f"Öffne Ressource: {self.resource_name}")
            self.resource = self._setup_resource()
            return self.resource
        except Exception as e:
            raise RuntimeError(f"Fehler beim Öffnen der Ressource: {str(e)}")

    def __exit__(self, exc_type, exc_value, traceback):
        """
        Wird beim Verlassen des Context Managers aufgerufen
        Stellt sicher, dass Ressourcen bereinigt werden
        
        Args:
            exc_type: Art der Exception (falls aufgetreten)
            exc_value: Wert der Exception
            traceback: Traceback-Objekt
        """
        try:
            if exc_type is not None:
                print(f"Exception aufgetreten: {exc_type.__name__}: {str(exc_value)}")
            
            if self.resource is not None:
                self._cleanup_resource()
                print(f"Ressource bereinigt: {self.resource_name}")
                
        finally:
            # Sensitive Daten werden immer bereinigt, unabhängig von Exceptions
            if self.sensitive_data is not None:
                self._secure_cleanup()
                print("Sensitive Daten sicher bereinigt")
            
            self.resource = None
            self.sensitive_data = None

    def _setup_resource(self):
        """
        Initialisiert die Ressource
        In einer realen Implementierung würde hier die tatsächliche Ressource erstellt
        """
        return f"Aktive Ressource: {self.resource_name}"

    def _cleanup_resource(self):
        """
        Bereinigt die Ressource
        In einer realen Implementierung würde hier die tatsächliche Ressource geschlossen
        """
        if self.resource is not None:
            # Ressourcen-spezifische Cleanup-Operationen
            pass

    def _secure_cleanup(self):
        """
        Führt sichere Bereinigung von sensitiven Daten durch
        """
        if self.sensitive_data is not None:
            # Überschreiben sensitiver Daten mit None
            self.sensitive_data = None


# Beispiel für die Verwendung
def demonstrate_resource_management():
    # Normaler Fall
    try:
        with ResourceManager("DatabaseConnection", sensitive_data="password123") as resource:
            print(f"Arbeite mit {resource}")
            # Simuliere normale Operationen
            print("Führe Datenbankoperationen durch...")
            
    except Exception as e:
        print(f"Fehler aufgetreten: {str(e)}")

    # Fall mit Exception
    try:
        with ResourceManager("NetworkConnection", sensitive_data="api_key") as resource:
            print(f"Arbeite mit {resource}")
            # Simuliere einen Fehler
            raise ValueError("Netzwerkfehler aufgetreten")
            
    except Exception as e:
        print(f"Fehler aufgetreten: {str(e)}")

if __name__ == "__main__":
    demonstrate_resource_management()