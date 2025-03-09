import struct
import socket
import time
from dataclasses import dataclass
from typing import Optional, Tuple, Any
from enum import Enum
import logging

class ProtocolError(Exception):
    """Basisklasse für Protokoll-bezogene Fehler"""
    pass

class ProtocolViolation(ProtocolError):
    """Wird bei Verletzung der Protokollspezifikation ausgelöst"""
    pass

class TimeoutError(ProtocolError):
    """Wird bei Zeitüberschreitung ausgelöst"""
    pass

class MessageType(Enum):
    HANDSHAKE = 1
    DATA = 2
    CONTROL = 3
    TERMINATE = 4

@dataclass
class Message:
    msg_type: MessageType
    payload: bytes
    sequence_number: int

class ProtocolHandler:
    HEADER_FORMAT = "!BIH"  # message_type(1), payload_length(4), sequence_number(2)
    HEADER_SIZE = struct.calcsize(HEADER_FORMAT)
    MAX_PAYLOAD_SIZE = 16384  # 16KB maximum payload size
    TIMEOUT_SECONDS = 5.0

    def __init__(self, socket: socket.socket):
        self.socket = socket
        self.socket.settimeout(self.TIMEOUT_SECONDS)
        self.sequence_number = 0
        self.logger = logging.getLogger(__name__)

    def _check_payload_size(self, size: int) -> None:
        """Überprüft, ob die Payload-Größe innerhalb der erlaubten Grenzen liegt"""
        if size > self.MAX_PAYLOAD_SIZE:
            raise ProtocolViolation(
                f"Payload size {size} exceeds maximum allowed size {self.MAX_PAYLOAD_SIZE}"
            )

    def _read_exactly(self, size: int) -> bytes:
        """Liest exakt die angegebene Anzahl von Bytes"""
        buffer = bytearray()
        try:
            while len(buffer) < size:
                chunk = self.socket.recv(size - len(buffer))
                if not chunk:
                    raise ProtocolViolation("Connection closed unexpectedly")
                buffer.extend(chunk)
        except socket.timeout:
            raise TimeoutError("Timeout while reading data")
        return bytes(buffer)

    def receive_message(self) -> Message:
        """
        Empfängt eine Nachricht vom Socket mit Überprüfungen
        """
        try:
            # Header einlesen
            header_data = self._read_exactly(self.HEADER_SIZE)
            msg_type, payload_length, seq_num = struct.unpack(self.HEADER_FORMAT, header_data)

            # Payload-Größe überprüfen
            self._check_payload_size(payload_length)

            # Nachrichtentyp validieren
            try:
                msg_type = MessageType(msg_type)
            except ValueError:
                raise ProtocolViolation(f"Invalid message type: {msg_type}")

            # Payload einlesen
            payload = self._read_exactly(payload_length)

            # Sequenznummer überprüfen
            if seq_num != (self.sequence_number + 1) % 65536:
                raise ProtocolViolation(
                    f"Invalid sequence number. Expected {(self.sequence_number + 1) % 65536}, got {seq_num}"
                )

            self.sequence_number = seq_num
            return Message(msg_type, payload, seq_num)

        except struct.error as e:
            raise ProtocolViolation(f"Invalid message format: {e}")

    def send_message(self, msg_type: MessageType, payload: bytes) -> None:
        """
        Sendet eine Nachricht über das Socket mit Überprüfungen
        """
        try:
            # Payload-Größe überprüfen
            self._check_payload_size(len(payload))

            # Sequenznummer erhöhen
            self.sequence_number = (self.sequence_number + 1) % 65536

            # Header erstellen und senden
            header = struct.pack(
                self.HEADER_FORMAT,
                msg_type.value,
                len(payload),
                self.sequence_number
            )

            # Atomares Senden von Header und Payload
            self.socket.sendall(header + payload)

        except socket.timeout:
            raise TimeoutError("Timeout while sending data")
        except socket.error as e:
            raise ProtocolError(f"Socket error while sending: {e}")

    def validate_payload(self, msg_type: MessageType, payload: bytes) -> bool:
        """
        Validiert die Payload entsprechend des Nachrichtentyps
        """
        try:
            if msg_type == MessageType.HANDSHAKE:
                # Beispiel für Handshake-Validierung
                return len(payload) == 32 and payload.startswith(b"HELLO")
            elif msg_type == MessageType.DATA:
                # Beispiel für Daten-Validierung
                return len(payload) > 0
            elif msg_type == MessageType.CONTROL:
                # Beispiel für Control-Nachricht-Validierung
                return len(payload) >= 2
            elif msg_type == MessageType.TERMINATE:
                return len(payload) == 0
            return False
        except Exception:
            return False

class SecureProtocolSerializer:
    """Klasse für sichere Serialisierung und Deserialisierung von Daten"""

    @staticmethod
    def serialize(data: Any) -> bytes:
        """
        Serialisiert Daten sicher in ein Binärformat
        """
        try:
            if isinstance(data, (int, float)):
                return struct.pack("!d", float(data))
            elif isinstance(data, str):
                encoded = data.encode('utf-8')
                return struct.pack(f"!I{len(encoded)}s", len(encoded), encoded)
            elif isinstance(data, bytes):
                return struct.pack(f"!I{len(data)}s", len(data), data)
            else:
                raise ValueError(f"Unsupported data type: {type(data)}")
        except struct.error as e:
            raise ProtocolError(f"Serialization error: {e}")

    @staticmethod
    def deserialize(data: bytes, expected_type: type) -> Any:
        """
        Deserialisiert Daten sicher aus einem Binärformat
        """
        try:
            if expected_type in (int, float):
                return struct.unpack("!d", data)[0]
            elif expected_type == str:
                length = struct.unpack("!I", data[:4])[0]
                return data[4:4+length].decode('utf-8')
            elif expected_type == bytes:
                length = struct.unpack("!I", data[:4])[0]
                return data[4:4+length]
            else:
                raise ValueError(f"Unsupported data type: {expected_type}")
        except (struct.error, UnicodeDecodeError) as e:
            raise ProtocolError(f"Deserialization error: {e}")