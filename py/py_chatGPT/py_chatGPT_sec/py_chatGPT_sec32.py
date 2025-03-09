import socket
import struct
import threading
import time
import pickle
from typing import Any

class ProtocolError(Exception):
    """Ausnahme für Protokollverletzungen."""
    pass

class TimeoutError(Exception):
    """Ausnahme für Timeouts."""
    pass

class CustomProtocolHandler:
    def __init__(self, socket: socket.socket, buffer_size: int = 1024, timeout: int = 10):
        self.socket = socket
        self.buffer_size = buffer_size
        self.timeout = timeout
        self.lock = threading.Lock()

    def send_message(self, message: Any) -> None:
        """Sendet eine Nachricht sicher über den Socket."""
        try:
            serialized_data = pickle.dumps(message)
            if len(serialized_data) > self.buffer_size:
                raise ValueError("Nachricht ist zu groß für den Puffer.")

            # Header: Länge der Nachricht (4 Bytes, unsigned int)
            header = struct.pack('!I', len(serialized_data))
            self.socket.sendall(header + serialized_data)
        except Exception as e:
            raise ProtocolError(f"Fehler beim Senden der Nachricht: {e}")

    def receive_message(self) -> Any:
        """Empfängt eine Nachricht sicher über den Socket."""
        self.socket.settimeout(self.timeout)
        try:
            # Header empfangen
            header = self._recv_exact(4)
            message_length = struct.unpack('!I', header)[0]

            if message_length > self.buffer_size:
                raise ProtocolError("Nachricht ist zu groß für den Puffer.")

            # Nachricht empfangen
            data = self._recv_exact(message_length)
            return pickle.loads(data)
        except socket.timeout:
            raise TimeoutError("Empfangen der Nachricht ist abgelaufen.")
        except Exception as e:
            raise ProtocolError(f"Fehler beim Empfangen der Nachricht: {e}")

    def _recv_exact(self, num_bytes: int) -> bytes:
        """Empfängt genau num_bytes vom Socket."""
        chunks = []
        bytes_received = 0

        while bytes_received < num_bytes:
            chunk = self.socket.recv(min(num_bytes - bytes_received, self.buffer_size))
            if not chunk:
                raise ProtocolError("Verbindung wurde geschlossen, bevor die Daten vollständig empfangen wurden.")
            chunks.append(chunk)
            bytes_received += len(chunk)

        return b''.join(chunks)

    def close(self):
        """Schließt den Socket."""
        with self.lock:
            try:
                self.socket.shutdown(socket.SHUT_RDWR)
            except Exception:
                pass
            finally:
                self.socket.close()

# Beispiel zur Verwendung des Protokoll-Handlers
if __name__ == "__main__":
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(("localhost", 12345))
    server_socket.listen(1)

    def server():
        conn, addr = server_socket.accept()
        handler = CustomProtocolHandler(conn)
        try:
            message = handler.receive_message()
            print(f"Server empfangen: {message}")
            handler.send_message({"status": "OK"})
        finally:
            handler.close()

    def client():
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_socket.connect(("localhost", 12345))
        handler = CustomProtocolHandler(client_socket)
        try:
            handler.send_message({"message": "Hallo Server"})
            response = handler.receive_message()
            print(f"Client empfangen: {response}")
        finally:
            handler.close()

    # Threads für Server und Client starten
    server_thread = threading.Thread(target=server)
    server_thread.start()

    time.sleep(1)  # Warten, bis der Server bereit ist

    client_thread = threading.Thread(target=client)
    client_thread.start()

    server_thread.join()
    client_thread.join()

    server_socket.close()
