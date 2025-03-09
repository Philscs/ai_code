import os
import hashlib
from typing import List, Dict

class ContainerImage:
    def __init__(self, name: str):
        self.name = name
        self.layers = {}
        self.dependencies = []

class LayerManager:
    def __init__(self):
        self.layers = {}

    def add_layer(self, layer_name: str, image_hash: str):
        if layer_name in self.layers:
            raise ValueError(f"Layer {layer_name} bereits existiert")
        self.layers[layer_name] = image_hash

    def remove_layer(self, layer_name: str):
        if layer_name not in self.layers:
            raise ValueError(f"Layer {layer_name} nicht gefunden")
        del self.layers[layer_name]

class DependencyResolver:
    def __init__(self, container_image: ContainerImage):
        self.container_image = container_image

    def add_dependency(self, dependency: ContainerImage):
        self.container_image.dependencies.append(dependency)

class Builder:
    def __init__(self, layer_manager: LayerManager, dependency_resolver: DependencyResolver):
        self.layer_manager = layer_manager
        self.dependency_resolver = dependency_resolver

    def build_image(self, container_image: ContainerImage):
        # Hier kommt die Logik für das Erstellen des Images
        # Zum Beispiel durch die Ausführung von Befehlen im Dockerfile
        print(f"Building image {container_image.name}...")
        for layer_name in container_image.layers:
            layer = self.layer_manager.get_layer(layer_name)
            if not layer:
                raise ValueError(f"Layer {layer_name} nicht gefunden")
            print(f"Loading layer {layer_name} from {layer}")
        for dependency in container_image.dependencies:
            self.dependency_resolver.add_dependency(container_image)
            print(f"Adding dependency {dependency.name} to image {container_image.name}")
        print("Image built successfully!")

class ContainerManager:
    def __init__(self):
        self.container_images = {}

    def add_container_image(self, name: str, layers: List[str], dependencies: 
List[ContainerImage]):
        container_image = ContainerImage(name)
        for layer_name in layers:
            container_image.layers[layer_name] = os.path.join("images", 
f"{name}-{layer_name}.tar")
        for dependency in dependencies:
            container_image.dependencies.append(dependency)
        self.container_images[name] = container_image

    def remove_container_image(self, name: str):
        if name not in self.container_images:
            raise ValueError(f"Container {name} nicht gefunden")
        del self.container_images[name]

class DockerfileExecutor:
    def __init__(self, builder: Builder):
        self.builder = builder

    def execute_dockerfile(self, dockerfile_path: str):
        # Hier kommt die Logik für das Ausführen des Dockerfiles
        with open(dockerfile_path, 'r') as f:
            instructions = [line.strip() for line in f.readlines()]
        print("Executing Dockerfile...")
        self.builder.build_image(self.container_image)

class HashManager:
    def __init__(self):
        self.image_hashes = {}

    def get_hash(self, image_name: str):
        if image_name not in self.image_hashes:
            raise ValueError(f"Image {image_name} nicht gefunden")
        return self.image_hashes[image_name]

    def update_hash(self, image_name: str, new_hash: str):
        self.image_hashes[image_name] = new_hash

def main():
    layer_manager = LayerManager()
    dependency_resolver = DependencyResolver(ContainerImage("my_image"))
    builder = Builder(layer_manager, dependency_resolver)
    container_manager = ContainerManager()

    # Hier kommt die Logik für das Erstellen der Images
    # Zum Beispiel durch die Übertragung von Befehlen aus dem Dockerfile
    dockerfile_path = "Dockerfile"
    with open(dockerfile_path, 'r') as f:
        instructions = [line.strip() for line in f.readlines()]
    
    print("Executing Dockerfile...")
    container_manager.add_container_image(
        name="my_image",
        layers=["layer1", "layer2"],
        dependencies=[ContainerImage("dependency1"), ContainerImage("dependency2")],
    )

    # Hier kommt die Logik für das Erstellen der Images
    builder.build_image(container_manager.container_images["my_image"])

if __name__ == "__main__":
    main()