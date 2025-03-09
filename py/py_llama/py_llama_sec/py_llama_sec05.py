import os
import glob
from cryptography.hazmat.primitives import padding
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from Crypto.PublicKey import RSA
from Crypto.Cipher import PKCS1_OAEP
from gpg import GPG

class BackupSystem:
    def __init__(self):
        self.schlüssel = None
        self.cloud_speicher = {}

    def initialisierung(self, cloud_speicher):
        # Erstelle einen neuen GnuPG-Schlüssel
        self.schlüssel = RSA.generate(2048)
        print("GnuPG-Schlüssel erstellt")

        # Speichere den Schlüssel auf dem Computer
        with open("schlüssel.gpg", "wb") as f:
            self.schlüssel.export_to_file(self.schlüssel, "OpenPGP")
            print("Schlüssel gespeichert")

    def verschlüsseln(self, datei):
        # Verschlüssle die Datei mit dem GnuPG-Schlüssel
        gpg = GPG()
        gpg.encrypt_file(datei, self.schlüssel)
        print("Datei verschlüsselt")

    def backup(self, cloud_speicher):
        # Durchläuft alle Dateien im Ordner und erstellt einen Backup-Ordner
        for datei in glob.glob("*.dat"):
            self.verschlüsseln(datei)

            # Erstelle einen Backup-Name für die Datei
            with open(datei, "r") as f:
                backup_name = "Backup_" + os.path.basename(f.name) + ".dat"
            print("Datei im Backup-Ordner speichern")

    def synchronisieren(self):
        # Durchläuft alle Cloud-Speicher und synchronisiert die Backups
        for cloud_speicher, url in self.cloud_speicher.items():
            gpg = GPG()
            gpg.decrypt_file(url + "/Backup_*", self.schlüssel)
            print("Cloud-Speichersynchronisation durchgeführt")

    def löschen(self):
        # Löse alle temporären Dateien
        os.system("rm -rf tmp*")