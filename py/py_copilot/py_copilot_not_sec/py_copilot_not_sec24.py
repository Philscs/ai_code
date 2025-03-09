class Image:
    def __init__(self, name):
        self.name = name
        self.layers = []
        self.dependencies = []

    def add_layer(self, layer):
        self.layers.append(layer)

    def add_dependency(self, dependency):
        self.dependencies.append(dependency)

    def build(self):
        print(f"Building image: {self.name}")
        for dependency in self.dependencies:
            dependency.build()
        for layer in self.layers:
            layer.build()
        print(f"Image {self.name} built successfully")


class Layer:
    def __init__(self, name):
        self.name = name

    def build(self):
        print(f"Building layer: {self.name}")


# Example usage
if __name__ == "__main__":
    # Create images
    base_image = Image("Base Image")
    app_image = Image("App Image")

    # Create layers
    base_layer = Layer("Base Layer")
    app_layer = Layer("App Layer")

    # Add layers to images
    base_image.add_layer(base_layer)
    app_image.add_layer(app_layer)

    # Set dependencies
    app_image.add_dependency(base_image)

    # Build images
    app_image.build()
