import socket
import threading
import logging
from cryptography.fernet import Fernet
from datetime import datetime
import hashlib
import os

# Generiere und speichere einen Verschlüsselungsschlüssel (falls nicht vorhanden)
KEY_FILE = "encryption.key"
if not os.path.exists(KEY_FILE):
    key = Fernet.generate_key()
    with open(KEY_FILE, 'wb') as key_file:
        key_file.write(key)
else:
    with open(KEY_FILE, 'rb') as key_file:
        key = key_file.read()

cipher = Fernet(key)

# Konfiguriere das Logging
LOG_FILE = "network_alerts.log"
logging.basicConfig(level=logging.INFO, format='%(message)s')
logger = logging.getLogger()
def encrypt_log(message):
    encrypted_message = cipher.encrypt(message.encode())
    with open(LOG_FILE, "ab") as log_file:
        log_file.write(encrypted_message + b"\n")

def authenticate_admin(password):
    # Sichere Passwort-Authentifizierung (SHA-256-Hashing)
    admin_password_hash = "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd7b8a6dc3b36010c8d"  # Hash für "password"
    input_hash = hashlib.sha256(password.encode()).hexdigest()
    return input_hash == admin_password_hash

def monitor_network():
    def handle_connection(conn, addr):
        logger.info(f"Verbindung von {addr} erhalten")
        while True:
            try:
                data = conn.recv(1024)
                if not data:
                    break

                # Analysiere TCP/IP-Pakete
                if b"suspicious_pattern" in data:
                    alert_message = f"{datetime.now()} - Verdächtiges Muster von {addr}: {data.decode(errors='ignore')}"
                    logger.info(alert_message)
                    encrypt_log(alert_message)
            except Exception as e:
                logger.error(f"Fehler bei der Verarbeitung von {addr}: {e}")
                break
        conn.close()

    # Socket-Setup
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server_socket.bind(("0.0.0.0", 9999))
        server_socket.listen(5)
        logger.info("Netzwerk-Monitor gestartet...")

        while True:
            conn, addr = server_socket.accept()
            thread = threading.Thread(target=handle_connection, args=(conn, addr))
            thread.daemon = True
            thread.start()

# Admin-Panel zur Anzeige verschlüsselter Logs
def admin_panel():
    while True:
        print("1. Zeige verschlüsselte Logs")
        print("2. Beende Admin-Panel")
        choice = input("Wähle eine Option: ")

        if choice == "1":
            with open(LOG_FILE, "rb") as log_file:
                for line in log_file:
                    try:
                        decrypted_message = cipher.decrypt(line.strip()).decode()
                        print(decrypted_message)
                    except Exception as e:
                        print(f"Fehler beim Entschlüsseln: {e}")
        elif choice == "2":
            break
        else:
            print("Ungültige Option. Bitte wähle erneut.")

# Hauptprogramm
def main():
    print("Netzwerküberwachungsprogramm gestartet.")
    threading.Thread(target=monitor_network, daemon=True).start()

    while True:
        print("1. Admin-Panel starten")
        print("2. Programm beenden")
        choice = input("Wähle eine Option: ")

        if choice == "1":
            password = input("Admin-Passwort: ")
            if authenticate_admin(password):
                admin_panel()
            else:
                print("Authentifizierung fehlgeschlagen.")
        elif choice == "2":
            print("Programm beendet.")
            break
        else:
            print("Ungültige Option. Bitte wähle erneut.")

if __name__ == "__main__":
    main()
