import socket
import struct
import select
from pickle import loads, dumps
from typing import Callable, Any

class ProtocolHandler:
    def __init__(self):
        self.max_buffer_size = 1024 * 1024  # 1 MB
        self.timeout = 10  # Sekunden

    def handle_protocol(self, sock: socket.socket, protocol: Callable[[bytes], Any]) -> None:
        while True:
            readable, writable, errored = select.select([sock], [], [sock])
            if sock in readable:
                try:
                    data = sock.recv(self.max_buffer_size)
                    if not data:
                        break
                    message = protocol(data)
                    self.check_protocol(message)
                except socket.timeout:
                    print("Timeout-Entzug!")
                    return
                except Exception as e:
                    print(f"Protokoll-Violation: {e}")
            elif sock in errored:
                print("Fehler beim Erhalten von Daten.")
                break

    def check_protocol(self, message: Any) -> None:
        if not isinstance(message, dict):
            raise ProtocolViolationError("Message must be a dictionary")
        required_keys = ["header", "payload"]
        for key in required_keys:
            if key not in message:
                raise ProtocolViolationError(f"Missing required field '{key}'")

    def serialize(self, data: Any) -> bytes:
        return dumps(data).encode()

    def deserialize(self, data: bytes) -> Any:
        return loads(data)

class ProtocolViolationError(Exception):
    pass

if __name__ == "__main__":
    # Erstellen eines neuen Socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    handler = ProtocolHandler()
    # Binden des Sockets an eine Adresse und ein Port
    sock.bind(("localhost", 8080))
    # Hören auf Verbindungen
    sock.listen(5)

    while True:
        conn, addr = sock.accept()
        print(f"Verbindung mit {addr} hergestellt!")
        handler.handle_protocol(conn, handler.deserialize)
