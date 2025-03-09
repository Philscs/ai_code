import os
import hashlib
import random
from copy import deepcopy
import time

class Verzeichnis:
    def __init__(self, name):
        self.name = name
        self.inhalt = ""
        self.replizate = {}

    def erstellen(self):
        if not os.path.exists(f"{self.name}"):
            os.mkdir(f"{self.name}")
        else:
            print("Verzeichnis bereits existiert.")

    def ersetzen(self, neuer_inhalt):
        with open(f"{self.name}/inhalt.txt", "w") as f:
            f.write(neuer_inhalt)
        self.inhalt = neuer_inhalt
        self.replizate["neu"] = deepcopy(self)

class ReplicatedFilesystem:
    def __init__(self):
        self.verzeichnisse = {}
        self.replikationen = {}

    def erstellen(self, verzeichnisname):
        if verzeichnisname in self.verzeichnisse:
            print("Verzeichnis bereits existiert.")
            return
        else:
            verzeichnis = Verzeichnis(verzeichnisname)
            self.verzeichnisse[verzeichnisname] = verzeichnis

    def ersetzen(self, verzeichnisname, neuer_inhalt):
        if verzeichnisname not in self.verzeichnisses:
            print("Verzeichnis nicht existiert.")
            return
        else:
            verzeichnis = self.verzeichnisse[verzeichnisname]
            verzeichnis.ersetzen(neuer_inhalt)

    def replizieren(self):
        for verzeichnis in self.verzeichnisses.values():
            verzeichnis.replizate["alt"] = deepcopy(verzeichnis)
            verzeichnis.inhalt += hashlib.sha256(verzeichnis.inhalt.encode()).hexdigest()
        for verzeichnis, altverzeichnis in self.replikationen.items():
            os.replace(f"{altverzeichnis.name}", f"{verzeichnis.name}")
        print("Replizierung erfolgreich.")

    def replizieren_manual(self):
        for verzeichnis in self.verzeichnisses.values():
            print(f"Repliziere Verzeichnis {verzeichnis.name}...")
            time.sleep(1)
            os.replace(f"{verzeichnis.replizate['alt'].name}", f"{verzeichnis.name}")

def main():
    fs = ReplicatedFilesystem()

    while True:
        print("\n1. Erstellen eines Neuen Verzeichnisses")
        print("2. ersetzen des Inhalts eines bestehenden Verzeichnisses")
        print("3. Replizieren des gesamten Dateisystems")
        print("4. Manuelle Replication")
        print("5. Beenden")
        
        Wahl = input("Wählen Sie eine Option: ")

        if Wahl == "1":
            name = input("Geben Sie den Namen des Verzeichnisses ein: ")
            fs.erstellen(name)
        elif Wahl == "2":
            name = input("Geben Sie den Namen des Verzeichnisses ein: ")
            neuer_inhalt = input("Geben Sie den neuen Inhalt ein: ")
            fs.ersetzen(name, neuer_inhalt)
        elif Wahl == "3":
            fs.replizieren()
        elif Wahl == "4":
            fs.replizieren_manual()
        elif Wahl == "5":
            break
        else:
            print("Ungültige Option")

if __name__ == "__main__":
    main()

