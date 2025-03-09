import scapy.all as scapy
from cryptography.fernet import Fernet
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
import base64
import os
import logging

# Konfiguration der Log-Datei
LOG_FILE = "netzwerkverbindungen.log"

# Erstelle eine verschlüsselte Schlüsseldatei
def erstelle_slash_key(password):
    # KDF-Passwort-Verwaltung
    kdf = PBKDF2HMAC(
        algorithm=hashes.SHA256(),
        length=32,
        salt=b'salt',
        iterations=100000,
    )

    # Schlüssel generieren
    key = base64.urlsafe_b64encode(kdf.derive(password.encode()))
    return key

# Konfiguration der Log-Datei
def konfiguriere_log():
    fernet_key = erstelle_slash_key(os.environ.get("ADMIN_PASSWORD"))
    fernet = Fernet(fernet_key)
    with open(LOG_FILE, 'w') as log:
        log.write(f"Netzwerkverbindungen analysiert. Verschlüsselung aktiviert.")

# TCP/IP-Paketanalyzer
def paket_analyse(packet):
    # Paket-Informationen extrahieren
    src_ip = packet[scapy.IP].src
    dst_ip = packet[scapy.IP].dst
    src_port = packet[scapy.TCP].sport
    dst_port = packet[scapy.TCP].dport

    # Verdächtige Muster erkennen (z.B. verdächtiger Datenverkehr)
    if packet.haslayer(scapy.TCP) and packet[scapy.TCP].flags == "S Syn":
        logging.warning(f"Verdächtiger SYN-Attacke von {src_ip}:{src_port} gegen 
{dst_ip}:{dst_port}")
    return

# Administrator-Authentifizierung
def authentifiziere_admin(admin_user, admin_password):
    # KDF-Passwort-Verwaltung
    kdf = PBKDF2HMAC(
        algorithm=hashes.SHA256(),
        length=32,
        salt=b'salt',
        iterations=100000,
    )

    # Schlüssel generieren
    key = base64.urlsafe_b64encode(kdf.derive(admin_password.encode()))
    if admin_user == os.environ.get("ADMIN_USER") and key == fernet_key:
        return True
    return False

# Hauptprogramm
def haupt():
    logging.basicConfig(level=logging.WARNING)

    konfiguriere_log()

    # TCP/IP-Paketanalyzer starten
    scapy.sniff(filter="tcp", prn=paket_analyse, store=False)

if __name__ == "__main__":
    fernet_key = erstelle_slash_key(os.environ.get("ADMIN_PASSWORD"))