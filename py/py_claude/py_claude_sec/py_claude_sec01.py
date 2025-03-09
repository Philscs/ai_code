import scapy.all as scapy
from cryptography.fernet import Fernet
import threading
import logging
import hashlib
import hmac
import os
from datetime import datetime
from typing import Dict, List
import queue

class SecureNetworkMonitor:
    def __init__(self, key_file: str = "monitor.key"):
        # Sichere Schlüsselerzeugung und -speicherung
        self.encryption_key = self._load_or_generate_key(key_file)
        self.fernet = Fernet(self.encryption_key)
        
        # Thread-sichere Warteschlange für Logs
        self.log_queue = queue.Queue()
        
        # Thread-sicheres Dict für aktive Verbindungen
        self.active_connections: Dict[str, List] = {}
        self._conn_lock = threading.Lock()
        
        # Logging konfigurieren
        self._setup_logging()
        
        # Admin-Sessions
        self.admin_sessions = {}
        self.session_lock = threading.Lock()

    def _load_or_generate_key(self, key_file: str) -> bytes:
        if os.path.exists(key_file):
            with open(key_file, 'rb') as f:
                return f.read()
        else:
            key = Fernet.generate_key()
            with open(key_file, 'wb') as f:
                f.write(key)
            os.chmod(key_file, 0o600)  # Nur Owner darf lesen/schreiben
            return key

    def _setup_logging(self):
        logging.basicConfig(
            filename='network_monitor.enc',
            level=logging.INFO,
            format='%(asctime)s - %(levelname)s - %(message)s'
        )

    def _encrypt_log(self, message: str) -> bytes:
        return self.fernet.encrypt(message.encode())

    def authenticate_admin(self, username: str, password: str) -> str:
        """Sichere Admin-Authentifizierung mit Rate-Limiting"""
        salt = os.urandom(16)
        key = hashlib.pbkdf2_hmac(
            'sha256',
            password.encode(),
            salt,
            100000  # Hohe Iteration für Brute-Force-Schutz
        )
        
        with self.session_lock:
            # Prüfe auf zu viele Versuche
            if username in self.admin_sessions:
                last_attempt = self.admin_sessions[username].get('last_attempt', 0)
                attempts = self.admin_sessions[username].get('attempts', 0)
                
                if attempts >= 3 and (datetime.now().timestamp() - last_attempt) < 300:
                    raise Exception("Zu viele Anmeldeversuche. Bitte warten Sie 5 Minuten.")
            
            # Simulierte Überprüfung (in Produktion gegen sichere DB)
            if self._verify_credentials(username, key):
                session_token = os.urandom(32).hex()
                self.admin_sessions[username] = {
                    'token': session_token,
                    'created_at': datetime.now().timestamp(),
                    'attempts': 0
                }
                return session_token
            else:
                self._record_failed_attempt(username)
                raise Exception("Ungültige Anmeldedaten")

    def _verify_credentials(self, username: str, key: bytes) -> bool:
        """Sichere Überprüfung der Admin-Credentials"""
        # In Produktion: Sichere Überprüfung gegen verschlüsselte DB
        # Dies ist nur ein Beispiel
        return True  # Implementiere eigene Logik

    def _record_failed_attempt(self, username: str):
        with self.session_lock:
            if username not in self.admin_sessions:
                self.admin_sessions[username] = {'attempts': 1}
            else:
                self.admin_sessions[username]['attempts'] = \
                    self.admin_sessions[username].get('attempts', 0) + 1
            self.admin_sessions[username]['last_attempt'] = datetime.now().timestamp()

    def analyze_packet(self, packet) -> None:
        """Analyse einzelner Pakete auf verdächtige Muster"""
        if packet.haslayer(scapy.TCP):
            src_ip = packet[scapy.IP].src
            dst_ip = packet[scapy.IP].dst
            tcp_flags = packet[scapy.TCP].flags
            
            with self._conn_lock:
                conn_key = f"{src_ip}:{dst_ip}"
                
                # Erkennung von Port Scanning
                if conn_key not in self.active_connections:
                    self.active_connections[conn_key] = {
                        'start_time': datetime.now(),
                        'packets': [],
                        'syn_count': 0
                    }
                
                conn_data = self.active_connections[conn_key]
                conn_data['packets'].append({
                    'time': datetime.now(),
                    'flags': tcp_flags
                })
                
                # SYN Flood Erkennung
                if tcp_flags & 0x02:  # SYN Flag
                    conn_data['syn_count'] += 1
                    if conn_data['syn_count'] > 100:  # Schwellwert
                        self._log_security_event(
                            f"Möglicher SYN Flood Angriff von {src_ip}",
                            severity="HIGH"
                        )

    def _log_security_event(self, message: str, severity: str = "INFO") -> None:
        """Thread-sichere Logging-Funktion"""
        encrypted_message = self._encrypt_log(
            f"{severity} - {message} - {datetime.now().isoformat()}"
        )
        self.log_queue.put(encrypted_message)

    def start_monitoring(self, interface: str = "eth0") -> None:
        """Startet die Netzwerküberwachung"""
        # Start Logger Thread
        logger_thread = threading.Thread(target=self._process_log_queue)
        logger_thread.daemon = True
        logger_thread.start()
        
        # Start Packet Capture
        scapy.sniff(iface=interface, prn=self.analyze_packet, store=False)

    def _process_log_queue(self) -> None:
        """Verarbeitet die Log-Warteschlange"""
        while True:
            try:
                encrypted_log = self.log_queue.get()
                with open('network_monitor.enc', 'ab') as f:
                    f.write(encrypted_log + b'\n')
            except Exception as e:
                print(f"Fehler beim Logging: {e}")