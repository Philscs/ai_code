import numpy as np
from PIL import Image
import os
from typing import Callable, List, Union, Dict
from abc import ABC, abstractmethod

class ImageProcessor:
    """Hauptklasse für die Bildverarbeitung."""
    
    def __init__(self):
        self.filters: Dict[str, ImageFilter] = {}
        self.transformations: Dict[str, ImageTransformation] = {}
        self._register_default_filters()
        self._register_default_transformations()
    
    def _register_default_filters(self):
        """Registriert die Standard-Filter."""
        self.register_filter("grayscale", GrayscaleFilter())
        self.register_filter("blur", BlurFilter())
        self.register_filter("sharpen", SharpenFilter())
        self.register_filter("edge_detection", EdgeDetectionFilter())
    
    def _register_default_transformations(self):
        """Registriert die Standard-Transformationen."""
        self.register_transformation("rotate", RotateTransformation())
        self.register_transformation("resize", ResizeTransformation())
        self.register_transformation("flip", FlipTransformation())
    
    def register_filter(self, name: str, filter_obj: 'ImageFilter'):
        """Registriert einen neuen Filter."""
        self.filters[name] = filter_obj
    
    def register_transformation(self, name: str, transform_obj: 'ImageTransformation'):
        """Registriert eine neue Transformation."""
        self.transformations[name] = transform_obj
    
    def process_image(self, image: np.ndarray, operations: List[dict]) -> np.ndarray:
        """Verarbeitet ein Bild mit einer Liste von Operationen."""
        result = image.copy()
        
        for operation in operations:
            op_type = operation.get('type')
            op_name = operation.get('name')
            params = operation.get('params', {})
            
            if op_type == 'filter':
                if op_name in self.filters:
                    result = self.filters[op_name].apply(result, **params)
            elif op_type == 'transform':
                if op_name in self.transformations:
                    result = self.transformations[op_name].apply(result, **params)
        
        return result
    
    def batch_process(self, input_dir: str, output_dir: str, operations: List[dict]):
        """Verarbeitet alle Bilder in einem Verzeichnis."""
        os.makedirs(output_dir, exist_ok=True)
        
        for filename in os.listdir(input_dir):
            if filename.lower().endswith(('.png', '.jpg', '.jpeg')):
                input_path = os.path.join(input_dir, filename)
                output_path = os.path.join(output_dir, f"processed_{filename}")
                
                image = np.array(Image.open(input_path))
                processed = self.process_image(image, operations)
                Image.fromarray(processed).save(output_path)

class ImageFilter(ABC):
    """Abstrakte Basisklasse für Filter."""
    
    @abstractmethod
    def apply(self, image: np.ndarray, **kwargs) -> np.ndarray:
        pass

class ImageTransformation(ABC):
    """Abstrakte Basisklasse für Transformationen."""
    
    @abstractmethod
    def apply(self, image: np.ndarray, **kwargs) -> np.ndarray:
        pass

class GrayscaleFilter(ImageFilter):
    def apply(self, image: np.ndarray, **kwargs) -> np.ndarray:
        return np.dot(image[...,:3], [0.2989, 0.5870, 0.1140])

class BlurFilter(ImageFilter):
    def apply(self, image: np.ndarray, kernel_size: int = 3, **kwargs) -> np.ndarray:
        kernel = np.ones((kernel_size, kernel_size)) / (kernel_size * kernel_size)
        return self._convolve(image, kernel)
    
    def _convolve(self, image: np.ndarray, kernel: np.ndarray) -> np.ndarray:
        if len(image.shape) == 3:
            result = np.zeros_like(image)
            for channel in range(image.shape[2]):
                result[:,:,channel] = np.convolve(image[:,:,channel], kernel, mode='same')
            return result
        return np.convolve(image, kernel, mode='same')

class SharpenFilter(ImageFilter):
    def apply(self, image: np.ndarray, strength: float = 1.0, **kwargs) -> np.ndarray:
        kernel = np.array([
            [0, -1, 0],
            [-1, 5, -1],
            [0, -1, 0]
        ]) * strength
        return BlurFilter()._convolve(image, kernel)

class EdgeDetectionFilter(ImageFilter):
    def apply(self, image: np.ndarray, **kwargs) -> np.ndarray:
        # Sobel-Operator
        kernel_x = np.array([[-1, 0, 1], [-2, 0, 2], [-1, 0, 1]])
        kernel_y = np.array([[-1, -2, -1], [0, 0, 0], [1, 2, 1]])
        
        if len(image.shape) == 3:
            image = GrayscaleFilter().apply(image)
        
        grad_x = BlurFilter()._convolve(image, kernel_x)
        grad_y = BlurFilter()._convolve(image, kernel_y)
        
        return np.sqrt(grad_x**2 + grad_y**2)

class RotateTransformation(ImageTransformation):
    def apply(self, image: np.ndarray, angle: float = 90, **kwargs) -> np.ndarray:
        return np.rot90(image, k=int(angle / 90))

class ResizeTransformation(ImageTransformation):
    def apply(self, image: np.ndarray, scale: float = 1.0, **kwargs) -> np.ndarray:
        new_size = tuple(int(dim * scale) for dim in image.shape[:2])
        return np.array(Image.fromarray(image).resize(new_size[::-1]))

class FlipTransformation(ImageTransformation):
    def apply(self, image: np.ndarray, horizontal: bool = True, **kwargs) -> np.ndarray:
        if horizontal:
            return np.fliplr(image)
        return np.flipud(image)

# Beispiel für einen benutzerdefinierten Filter
class CustomFilter(ImageFilter):
    def __init__(self, filter_function: Callable):
        self.filter_function = filter_function
    
    def apply(self, image: np.ndarray, **kwargs) -> np.ndarray:
        return self.filter_function(image, **kwargs)

# Beispiel für die Verwendung:
if __name__ == "__main__":
    # Bildverarbeitungssystem initialisieren
    processor = ImageProcessor()
    
    # Beispiel für einen benutzerdefinierten Filter
    def sepia_filter(image: np.ndarray, intensity: float = 1.0) -> np.ndarray:
        sepia_matrix = np.array([
            [0.393 + 0.607 * (1 - intensity), 0.769 - 0.769 * (1 - intensity), 0.189 - 0.189 * (1 - intensity)],
            [0.349 - 0.349 * (1 - intensity), 0.686 + 0.314 * (1 - intensity), 0.168 - 0.168 * (1 - intensity)],
            [0.272 - 0.272 * (1 - intensity), 0.534 - 0.534 * (1 - intensity), 0.131 + 0.869 * (1 - intensity)]
        ])
        return np.dot(image[...,:3], sepia_matrix.T)
    
    # Benutzerdefinierten Filter registrieren
    processor.register_filter("sepia", CustomFilter(sepia_filter))
    
    # Beispiel für eine Verarbeitungspipeline
    operations = [
        {"type": "filter", "name": "grayscale"},
        {"type": "transform", "name": "resize", "params": {"scale": 0.5}},
        {"type": "filter", "name": "edge_detection"},
    ]
    
    # Einzelbild verarbeiten
    input_image = np.array(Image.open("input.jpg"))
    processed_image = processor.process_image(input_image, operations)
    Image.fromarray(processed_image.astype(np.uint8)).save("output.jpg")
    
    # Batch-Verarbeitung
    processor.batch_process("input_directory", "output_directory", operations)