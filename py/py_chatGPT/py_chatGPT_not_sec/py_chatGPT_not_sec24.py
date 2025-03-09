import os
import hashlib
from typing import List, Dict

# Klasse zur Repräsentation eines Containers
class ContainerLayer:
    def __init__(self, name: str, dependencies: List[str], commands: List[str]):
        self.name = name
        self.dependencies = dependencies
        self.commands = commands
        self.hash = None

    def calculate_hash(self):
        """Berechnet einen eindeutigen Hash basierend auf den Commands und Dependencies."""
        hash_data = ''.join(self.dependencies + self.commands).encode('utf-8')
        self.hash = hashlib.sha256(hash_data).hexdigest()
        return self.hash

class ContainerImageManager:
    def __init__(self):
        self.layers: Dict[str, ContainerLayer] = {}
        self.build_cache: Dict[str, str] = {}

    def add_layer(self, name: str, dependencies: List[str], commands: List[str]):
        """Fügt einen neuen Layer hinzu."""
        if name in self.layers:
            raise ValueError(f"Layer {name} existiert bereits!")

        for dep in dependencies:
            if dep not in self.layers:
                raise ValueError(f"Dependency {dep} nicht gefunden!")

        layer = ContainerLayer(name, dependencies, commands)
        layer.calculate_hash()
        self.layers[name] = layer

    def resolve_dependencies(self, layer_name: str) -> List[str]:
        """Löst die Abhängigkeiten eines Layers rekursiv auf."""
        if layer_name not in self.layers:
            raise ValueError(f"Layer {layer_name} nicht gefunden!")

        resolved = []
        visited = set()

        def _resolve(name):
            if name in visited:
                return
            visited.add(name)
            for dep in self.layers[name].dependencies:
                _resolve(dep)
            resolved.append(name)

        _resolve(layer_name)
        return resolved

    def build_layer(self, layer_name: str):
        """Baut einen Layer und seine Abhängigkeiten."""
        if layer_name not in self.layers:
            raise ValueError(f"Layer {layer_name} nicht gefunden!")

        build_order = self.resolve_dependencies(layer_name)
        for name in build_order:
            layer = self.layers[name]
            if layer.hash in self.build_cache:
                print(f"Layer {name} ist bereits gebaut. Überspringen...")
            else:
                print(f"Baue Layer {name}...")
                for cmd in layer.commands:
                    print(f"  Führe aus: {cmd}")
                self.build_cache[layer.hash] = name

    def list_layers(self):
        """Listet alle registrierten Layers auf."""
        print("Verfügbare Layers:")
        for name, layer in self.layers.items():
            print(f"- {name} (Hash: {layer.hash})")

# Beispielnutzung
if __name__ == "__main__":
    manager = ContainerImageManager()

    # Layers definieren
    manager.add_layer("base", [], ["apt-get update", "apt-get install -y python3"])
    manager.add_layer("app", ["base"], ["pip install flask", "copy . /app"])
    manager.add_layer("test", ["app"], ["pytest"])

    # Layers auflisten
    manager.list_layers()

    # Layer bauen
    print("\nBaue Layer 'test':")
    manager.build_layer("test")
