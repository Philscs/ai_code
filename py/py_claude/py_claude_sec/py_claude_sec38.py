from typing import Any, Dict, Set, Type, Optional
import json
import pickle
from dataclasses import dataclass, asdict
from datetime import datetime
import hashlib
import logging

class SerializerException(Exception):
    pass

class VersionMismatchError(SerializerException):
    pass

class CyclicReferenceError(SerializerException):
    pass

@dataclass
class SerializerMetadata:
    version: str = "1.0"
    created_at: str = datetime.now().isoformat()
    checksum: str = ""

class SafeSerializer:
    def __init__(self, version: str = "1.0", allowed_types: Optional[Set[Type]] = None):
        self.version = version
        self.allowed_types = allowed_types or {int, float, str, bool, list, dict, datetime}
        self.seen_objects: Set[int] = set()
        self.logger = logging.getLogger(__name__)

    def _create_metadata(self, data: str) -> SerializerMetadata:
        """Erstellt Metadata für serialisierte Daten"""
        metadata = SerializerMetadata(
            version=self.version,
            created_at=datetime.now().isoformat()
        )
        metadata.checksum = hashlib.sha256(data.encode()).hexdigest()
        return metadata

    def _verify_type(self, obj: Any) -> None:
        """Überprüft, ob der Objekttyp zur Serialisierung zugelassen ist"""
        obj_type = type(obj)
        if obj_type not in self.allowed_types:
            raise SerializerException(
                f"Typ {obj_type} ist nicht zur Serialisierung zugelassen"
            )

    def _detect_cycles(self, obj: Any) -> None:
        """Erkennt zyklische Referenzen im Objekt"""
        obj_id = id(obj)
        if obj_id in self.seen_objects:
            raise CyclicReferenceError("Zyklische Referenz erkannt")
        self.seen_objects.add(obj_id)

    def _serialize_object(self, obj: Any) -> Dict:
        """Serialisiert ein einzelnes Objekt"""
        self._verify_type(obj)
        self._detect_cycles(obj)

        if isinstance(obj, datetime):
            return {
                "__type__": "datetime",
                "value": obj.isoformat()
            }
        elif hasattr(obj, "__dict__"):
            return {
                "__type__": obj.__class__.__name__,
                "value": {k: self._serialize_object(v) for k, v in obj.__dict__.items()}
            }
        elif isinstance(obj, (list, tuple)):
            return [self._serialize_object(item) for item in obj]
        elif isinstance(obj, dict):
            return {k: self._serialize_object(v) for k, v in obj.items()}
        return obj

    def serialize(self, obj: Any) -> str:
        """Hauptmethode zur Serialisierung von Objekten"""
        try:
            self.seen_objects.clear()
            serialized_data = self._serialize_object(obj)
            
            result = {
                "data": serialized_data,
                "metadata": asdict(self._create_metadata(json.dumps(serialized_data)))
            }
            
            return json.dumps(result, indent=2)
        except Exception as e:
            self.logger.error(f"Serialisierungsfehler: {str(e)}")
            raise

    def _deserialize_object(self, data: Any) -> Any:
        """Deserialisiert ein einzelnes Objekt"""
        if isinstance(data, dict) and "__type__" in data:
            obj_type = data["__type__"]
            if obj_type == "datetime":
                return datetime.fromisoformat(data["value"])
            # Hier können weitere benutzerdefinierte Typen hinzugefügt werden
            raise SerializerException(f"Unbekannter Typ: {obj_type}")
        elif isinstance(data, list):
            return [self._deserialize_object(item) for item in data]
        elif isinstance(data, dict):
            return {k: self._deserialize_object(v) for k, v in data.items()}
        return data

    def deserialize(self, data_str: str, verify_checksum: bool = True) -> Any:
        """Hauptmethode zur Deserialisierung von Objekten"""
        try:
            parsed_data = json.loads(data_str)
            
            # Metadaten überprüfen
            metadata = parsed_data.get("metadata", {})
            if metadata.get("version") != self.version:
                raise VersionMismatchError(
                    f"Version mismatch: {metadata.get('version')} != {self.version}"
                )
            
            # Checksum verifizieren
            if verify_checksum:
                data_checksum = hashlib.sha256(
                    json.dumps(parsed_data["data"]).encode()
                ).hexdigest()
                if data_checksum != metadata.get("checksum"):
                    raise SerializerException("Checksum validation failed")
            
            return self._deserialize_object(parsed_data["data"])
            
        except json.JSONDecodeError as e:
            self.logger.error(f"JSON Dekodierungsfehler: {str(e)}")
            raise SerializerException("Ungültiges JSON Format")
        except Exception as e:
            self.logger.error(f"Deserialisierungsfehler: {str(e)}")
            raise

# Beispiel für die Verwendung
if __name__ == "__main__":
    # Beispielklasse für verschachtelte Objekte
    @dataclass
    class Person:
        name: str
        birth_date: datetime
        friends: list

    # Serializer erstellen
    serializer = SafeSerializer(version="1.0")
    
    # Testdaten erstellen
    person = Person(
        name="Max Mustermann",
        birth_date=datetime.now(),
        friends=["Anna", "Bob"]
    )
    
    try:
        # Serialisierung
        serialized = serializer.serialize(person)
        print("Serialisierte Daten:", serialized)
        
        # Deserialisierung
        deserialized = serializer.deserialize(serialized)
        print("Deserialisierte Daten:", deserialized)
        
    except SerializerException as e:
        print(f"Fehler bei der Serialisierung/Deserialisierung: {str(e)}")