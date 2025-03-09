import subprocess
import shlex
import resource
import os
from typing import List, Optional

class SecureSubprocess:
    def __init__(self, timeout: Optional[int] = None, 
                 memory_limit_mb: Optional[int] = None):
        """
        Initialisiert die Wrapper-Klasse.

        :param timeout: Maximale Ausführungszeit in Sekunden.
        :param memory_limit_mb: Maximales Memory-Limit in MB für den Prozess.
        """
        self.timeout = timeout
        self.memory_limit_mb = memory_limit_mb

    def _set_resource_limits(self):
        """
        Setzt Ressourcenlimits für den Prozess.
        """
        if self.memory_limit_mb is not None:
            memory_limit_bytes = self.memory_limit_mb * 1024 * 1024
            resource.setrlimit(resource.RLIMIT_AS, (memory_limit_bytes, memory_limit_bytes))

    def run(self, command: List[str], env: Optional[dict] = None) -> subprocess.CompletedProcess:
        """
        Führt einen sicheren subprocess-Aufruf aus.

        :param command: Liste der Befehle und Argumente (nicht als String, um Shell-Injection zu vermeiden).
        :param env: Optionale Umgebungsvariablen als Dictionary.
        :return: subprocess.CompletedProcess-Objekt.
        """
        if not isinstance(command, list) or not all(isinstance(arg, str) for arg in command):
            raise ValueError("Command muss eine Liste von Strings sein.")

        # Sicherstellen, dass keine gefährlichen Umgebungsvariablen vorhanden sind
        safe_env = os.environ.copy()
        if env is not None:
            safe_env.update({k: v for k, v in env.items() if k.isalnum()})

        try:
            result = subprocess.run(
                command,
                env=safe_env,
                preexec_fn=self._set_resource_limits,  # Setzt Ressourcenlimits
                timeout=self.timeout,
                check=True,
                capture_output=True,
                text=True
            )
            return result
        except subprocess.TimeoutExpired as e:
            print(f"Timeout überschritten: {e}")
            raise
        except subprocess.CalledProcessError as e:
            print(f"Befehl fehlgeschlagen: {e}")
            raise
        except Exception as e:
            print(f"Unerwarteter Fehler: {e}")
            raise

# Beispielverwendung
def main():
    # Wrapper mit Timeout von 5 Sekunden und Speicherlimit von 100 MB
    wrapper = SecureSubprocess(timeout=5, memory_limit_mb=100)

    try:
        result = wrapper.run(["ls", "-l"], env={"LANG": "C"})
        print("Ausgabe:", result.stdout)
    except Exception as e:
        print(f"Fehler: {e}")

if __name__ == "__main__":
    main()
