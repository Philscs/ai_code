import json
import uuid
from typing import Any, Dict, Type

class SafeSerializer:
    def __init__(self, version: str = "1.0"):
        self.version = version
        self.id_map = {}
        self.allowed_types = {dict, list, str, int, float, bool, type(None)}

    def _get_object_id(self, obj: Any) -> str:
        if id(obj) not in self.id_map:
            self.id_map[id(obj)] = str(uuid.uuid4())
        return self.id_map[id(obj)]

    def _serialize_object(self, obj: Any, seen: set) -> Any:
        if id(obj) in seen:
            return {"__ref__": self._get_object_id(obj)}

        seen.add(id(obj))
        if isinstance(obj, dict):
            return {
                "__id__": self._get_object_id(obj),
                "__type__": "dict",
                "data": {key: self._serialize_object(value, seen) for key, value in obj.items()}
            }
        elif isinstance(obj, list):
            return {
                "__id__": self._get_object_id(obj),
                "__type__": "list",
                "data": [self._serialize_object(item, seen) for item in obj]
            }
        elif type(obj) in self.allowed_types:
            return obj
        else:
            raise ValueError(f"Unsupported type: {type(obj)}")

    def serialize(self, obj: Any) -> str:
        self.id_map.clear()
        serialized_data = {
            "__version__": self.version,
            "__root__": self._serialize_object(obj, set())
        }
        return json.dumps(serialized_data)

    def _deserialize_object(self, obj: Any, ref_map: Dict[str, Any]) -> Any:
        if isinstance(obj, dict) and "__type__" in obj:
            obj_id = obj.get("__id__")
            obj_type = obj["__type__"]

            if obj_id and obj_id in ref_map:
                return ref_map[obj_id]

            if obj_type == "dict":
                deserialized = {}
                ref_map[obj_id] = deserialized
                deserialized.update({key: self._deserialize_object(value, ref_map) for key, value in obj["data"].items()})
                return deserialized
            elif obj_type == "list":
                deserialized = []
                ref_map[obj_id] = deserialized
                deserialized.extend([self._deserialize_object(item, ref_map) for item in obj["data"]])
                return deserialized
            else:
                raise ValueError(f"Unsupported object type during deserialization: {obj_type}")

        elif isinstance(obj, dict) and "__ref__" in obj:
            ref_id = obj["__ref__"]
            if ref_id not in ref_map:
                raise ValueError(f"Invalid reference ID: {ref_id}")
            return ref_map[ref_id]

        elif type(obj) in self.allowed_types:
            return obj
        else:
            raise ValueError(f"Unsupported type during deserialization: {type(obj)}")

    def deserialize(self, serialized_str: str) -> Any:
        try:
            data = json.loads(serialized_str)
            if "__version__" not in data or "__root__" not in data:
                raise ValueError("Invalid serialized data format")

            if data["__version__"] != self.version:
                raise ValueError(f"Version mismatch: {data['__version__']} != {self.version}")

            ref_map = {}
            return self._deserialize_object(data["__root__"], ref_map)
        except (json.JSONDecodeError, ValueError) as e:
            raise ValueError(f"Deserialization failed: {e}")

# Beispielnutzung
serializer = SafeSerializer(version="1.0")
data = {"a": [1, 2, 3], "b": {"c": 4}}
data["b"]["ref_to_a"] = data["a"]  # Zyklische Referenz

# Serialisieren
serialized = serializer.serialize(data)
print("Serialized:", serialized)

# Deserialisieren
deserialized = serializer.deserialize(serialized)
print("Deserialized:", deserialized)
