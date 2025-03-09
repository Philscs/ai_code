from typing import Dict, List, Set, Optional
from dataclasses import dataclass
import hashlib
import json
import os
from datetime import datetime
import logging
import subprocess
from pathlib import Path

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

@dataclass
class Layer:
    """Repräsentiert einen einzelnen Container Layer"""
    layer_id: str
    parent_id: Optional[str]
    commands: List[str]
    created_at: datetime
    size: int
    checksum: str

    def to_dict(self) -> dict:
        return {
            'layer_id': self.layer_id,
            'parent_id': self.parent_id,
            'commands': self.commands,
            'created_at': self.created_at.isoformat(),
            'size': self.size,
            'checksum': self.checksum
        }

@dataclass
class Image:
    """Repräsentiert ein komplettes Container Image"""
    image_id: str
    name: str
    tag: str
    layers: List[Layer]
    dependencies: Set[str]
    
    def to_dict(self) -> dict:
        return {
            'image_id': self.image_id,
            'name': self.name,
            'tag': self.tag,
            'layers': [layer.to_dict() for layer in self.layers],
            'dependencies': list(self.dependencies)
        }

class ContainerRegistry:
    """Verwaltung der Container-Images und ihrer Layer"""
    
    def __init__(self, storage_path: str):
        self.storage_path = Path(storage_path)
        self.storage_path.mkdir(parents=True, exist_ok=True)
        self.images: Dict[str, Image] = {}
        self.layers: Dict[str, Layer] = {}
        self._load_registry()

    def _load_registry(self):
        """Lädt bestehende Registry-Daten aus dem Speicher"""
        registry_file = self.storage_path / 'registry.json'
        if registry_file.exists():
            with open(registry_file, 'r') as f:
                data = json.load(f)
                for image_data in data.get('images', []):
                    layers = []
                    for layer_data in image_data['layers']:
                        layer = Layer(
                            layer_id=layer_data['layer_id'],
                            parent_id=layer_data['parent_id'],
                            commands=layer_data['commands'],
                            created_at=datetime.fromisoformat(layer_data['created_at']),
                            size=layer_data['size'],
                            checksum=layer_data['checksum']
                        )
                        layers.append(layer)
                        self.layers[layer.layer_id] = layer
                    
                    image = Image(
                        image_id=image_data['image_id'],
                        name=image_data['name'],
                        tag=image_data['tag'],
                        layers=layers,
                        dependencies=set(image_data['dependencies'])
                    )
                    self.images[image.image_id] = image

    def _save_registry(self):
        """Speichert Registry-Daten persistent"""
        registry_file = self.storage_path / 'registry.json'
        with open(registry_file, 'w') as f:
            json.dump({
                'images': [image.to_dict() for image in self.images.values()]
            }, f, indent=2)

    def create_layer(self, commands: List[str], parent_id: Optional[str] = None) -> Layer:
        """Erstellt einen neuen Layer basierend auf den gegebenen Befehlen"""
        layer_content = json.dumps(commands, sort_keys=True).encode()
        layer_id = hashlib.sha256(layer_content).hexdigest()
        
        if layer_id in self.layers:
            return self.layers[layer_id]
        
        layer = Layer(
            layer_id=layer_id,
            parent_id=parent_id,
            commands=commands,
            created_at=datetime.now(),
            size=len(layer_content),
            checksum=hashlib.sha256(layer_content).hexdigest()
        )
        self.layers[layer_id] = layer
        return layer

    def create_image(self, name: str, tag: str, layers: List[Layer], dependencies: Set[str]) -> Image:
        """Erstellt ein neues Image mit den angegebenen Layern und Abhängigkeiten"""
        image_content = json.dumps({
            'name': name,
            'tag': tag,
            'layers': [layer.layer_id for layer in layers],
            'dependencies': sorted(list(dependencies))
        }, sort_keys=True).encode()
        
        image_id = hashlib.sha256(image_content).hexdigest()
        
        image = Image(
            image_id=image_id,
            name=name,
            tag=tag,
            layers=layers,
            dependencies=dependencies
        )
        self.images[image_id] = image
        self._save_registry()
        return image

    def resolve_dependencies(self, dependencies: Set[str]) -> List[Image]:
        """Löst alle Abhängigkeiten eines Images auf"""
        resolved = []
        visited = set()

        def resolve(dep: str):
            if dep in visited:
                return
            visited.add(dep)
            
            for image in self.images.values():
                if image.name == dep:
                    for sub_dep in image.dependencies:
                        resolve(sub_dep)
                    resolved.append(image)

        for dep in dependencies:
            resolve(dep)

        return resolved

    def build_image(self, dockerfile_path: str, tag: str) -> Image:
        """Baut ein neues Image aus einem Dockerfile"""
        logger.info(f"Building image from {dockerfile_path} with tag {tag}")
        
        # Parse Dockerfile
        with open(dockerfile_path, 'r') as f:
            dockerfile_content = f.readlines()

        layers = []
        current_commands = []
        dependencies = set()
        
        for line in dockerfile_content:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
                
            if line.upper().startswith('FROM '):
                # Neue Base-Image-Abhängigkeit
                base_image = line.split()[1]
                dependencies.add(base_image)
                
                if current_commands:
                    # Erstelle Layer aus bisherigen Kommandos
                    parent_id = layers[-1].layer_id if layers else None
                    layer = self.create_layer(current_commands, parent_id)
                    layers.append(layer)
                    current_commands = []
                    
            else:
                current_commands.append(line)
        
        # Letzten Layer erstellen
        if current_commands:
            parent_id = layers[-1].layer_id if layers else None
            layer = self.create_layer(current_commands, parent_id)
            layers.append(layer)

        # Image erstellen
        name = tag.split(':')[0]
        image = self.create_image(name, tag, layers, dependencies)
        
        logger.info(f"Successfully built image {image.image_id} with {len(layers)} layers")
        return image

    def get_image(self, image_id: str) -> Optional[Image]:
        """Gibt ein Image anhand seiner ID zurück"""
        return self.images.get(image_id)

    def get_layer(self, layer_id: str) -> Optional[Layer]:
        """Gibt einen Layer anhand seiner ID zurück"""
        return self.layers.get(layer_id)

    def list_images(self) -> List[Image]:
        """Gibt eine Liste aller verfügbaren Images zurück"""
        return list(self.images.values())

    def delete_image(self, image_id: str):
        """Löscht ein Image und seine nicht mehr benötigten Layer"""
        if image_id not in self.images:
            raise ValueError(f"Image {image_id} not found")
            
        image = self.images[image_id]
        used_layers = set()
        
        # Sammle alle Layer, die noch von anderen Images verwendet werden
        for other_image in self.images.values():
            if other_image.image_id != image_id:
                for layer in other_image.layers:
                    used_layers.add(layer.layer_id)
        
        # Lösche nicht mehr benötigte Layer
        for layer in image.layers:
            if layer.layer_id not in used_layers:
                del self.layers[layer.layer_id]
        
        del self.images[image_id]
        self._save_registry()
        logger.info(f"Deleted image {image_id}")
