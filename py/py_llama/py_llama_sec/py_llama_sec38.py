import json
from typing import Any, Dict, List
from uuid import UUID
from dataclasses import dataclass
from inspect import getmembers, isfunction
from collections import defaultdict

# Define eine Liste von Objekten, die nicht serialisiert werden sollen (z.B. Metadaten)
EXCEPTED_TYPES = [dict, type(None)]

@dataclass
class SerializerOptions:
    exclude: list  # Liste der Exklusionsobjekte
    version: int  = 1  # Version des Seriales

# Erstelle eine Dictionary-Struktur, die zyklische Referenzen verhindert
def create_dict_without_cycles(dictionary: Dict):
    visited = set()
    def helper(obj):
        if id(obj) in visited:
            return None  # Zyklische Referenz
        visited.add(id(obj))
        for key, value in obj.items():
            obj[key] = helper(value)
        return obj
    return helper(dictionary)

# Erstelle einen Serializer
class Serializer:
    def __init__(self, options: SerializerOptions):
        self.options = options

    # serialise Objekt und schützen es vor Object Injection
    def serialize(self, obj: object) -> str:
        # Verhindere zyklische Referenzen
        obj_dict = create_dict_without_cycles(obj.__dict__)

        # Exkludiere bestimmte Typen (z.B. Metadaten)
        for key in list(obj_dict.keys()):
            if type(obj[key]) in EXCEPTED_TYPES:
                del obj_dict[key]

        # Serialize die Daten
        serialized_data = json.dumps(obj_dict, default=lambda x: x.__dict__, 
cls=SerializerEncoder)

        return serialized_data

# Eine spezielle Klasse für den Encoder, um sicherzustellen, dass keine zyklischen Referenzen entstehen
class SerializerEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, (UUID)):
            return str(obj)
        elif hasattr(obj, '__dict__'):
            return obj.__dict__
        else:
            return super().default(obj)

# Verhindere Object Injection durch Prüfung der übergebenen Daten
class SafeDeserializer(Serializer):
    def deserialize(self, data: str) -> object:
        try:
            deserialized_data = json.loads(data)
            if 'version' not in deserialized_data or deserialized_data['version'] != self.options.version:
                raise ValueError("Version mismatch")
            return deserialized_data
        except json.JSONDecodeError as e:
            print(f"JSON Decode Error: {e}")
            return None

# Beispiel für die Verwendung
@dataclass
class User:
    id: UUID
    name: str
    email: str

# Verwende den Serializer
user = User(id=UUID('123456-7890-1234-5678-9012'), name='John Doe', email='john@example.com')
serializer = Serializer(SerializerOptions(exclude=['email']))
serialized_user = serializer.serialize(user)
print(serialized_user)

deserializer = SafeDeserializer(serializer.options)
deserialized_user = deserializer.deserialize(serialized_user)
print(deserialized_user.id, deserialized_user.name, deserialized_user.email)