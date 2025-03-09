import subprocess
from subprocess import PIPE, TimeoutExpired
import os
import shutil

class SafeSubprocess(subprocess.Subprocess):
    def __init__(self, command, timeout=60, resource_limit=None):
        super().__init__(command, shell=False)
        self.timeout = timeout
        if resource_limit:
            # Set Ressourcenlimits (max. CPU- und Memory-Ressourcen pro Prozess)
            self.setrlimit((resource_limit[0], resource_limit[1]))

    def execute(self):
        try:
            output, _ = self.communicate(timeout=self.timeout)
            return output.decode('utf-8')
        except TimeoutExpired as e:
            raise RuntimeError(f"Timeout von {self.timeout} Sekunden erreicht") from e

def safe_execute(command, timeout=60, resource_limit=None):
    if isinstance(command, str):
        # Wenn Command ein string ist, verwende die SafeSubprocess-Klasse
        return SafeSubprocess(command, timeout=timeout, resource_limit=resource_limit).execute()
    elif callable(command):
        # Wenn Command ein callable ist, führe es direkt aus und verwende subprocess.call
        output = subprocess.call(command, shell=False, timeout=timeout)
        if output != 0:
            raise RuntimeError(f"Command failed with exitcode {output}")
        return ''
    else:
        raise TypeError("Invalid command type. Expected str or callable.")