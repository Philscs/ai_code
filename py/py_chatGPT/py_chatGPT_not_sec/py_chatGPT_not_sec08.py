import cv2
import numpy as np
from typing import Callable, List, Tuple
import os

class ImageProcessingSystem:
    def __init__(self):
        self.custom_filters = {}

    def add_custom_filter(self, name: str, function: Callable[[np.ndarray], np.ndarray]):
        """Add a custom filter to the system."""
        self.custom_filters[name] = function

    def apply_filter(self, image: np.ndarray, filter_name: str, *args, **kwargs) -> np.ndarray:
        """Apply a predefined or custom filter to an image."""
        if filter_name == "grayscale":
            return cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
        elif filter_name == "blur":
            return cv2.GaussianBlur(image, (5, 5), 0)
        elif filter_name == "edge_detection":
            return cv2.Canny(image, 100, 200)
        elif filter_name in self.custom_filters:
            return self.custom_filters[filter_name](image)
        else:
            raise ValueError(f"Filter '{filter_name}' is not defined.")

    def resize(self, image: np.ndarray, size: Tuple[int, int]) -> np.ndarray:
        """Resize an image to the given size."""
        return cv2.resize(image, size)

    def rotate(self, image: np.ndarray, angle: float) -> np.ndarray:
        """Rotate an image by the given angle."""
        h, w = image.shape[:2]
        center = (w // 2, h // 2)
        matrix = cv2.getRotationMatrix2D(center, angle, 1.0)
        return cv2.warpAffine(image, matrix, (w, h))

    def analyze(self, image: np.ndarray) -> dict:
        """Analyze the image and return basic properties."""
        return {
            "shape": image.shape,
            "mean_color": image.mean(axis=(0, 1)),
            "min_color": image.min(axis=(0, 1)),
            "max_color": image.max(axis=(0, 1))
        }

    def batch_process(self, image_paths: List[str], output_dir: str, operations: List[Callable[[np.ndarray], np.ndarray]]):
        """Apply a series of operations to a batch of images and save the results."""
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)

        for image_path in image_paths:
            image = cv2.imread(image_path)
            if image is None:
                print(f"Warning: Could not read image {image_path}")
                continue

            processed_image = image
            for operation in operations:
                processed_image = operation(processed_image)

            filename = os.path.basename(image_path)
            output_path = os.path.join(output_dir, filename)
            cv2.imwrite(output_path, processed_image)

# Beispiel: Nutzung des Systems
if __name__ == "__main__":
    system = ImageProcessingSystem()

    # Benutzerdefinierter Filter
    def invert_colors(image: np.ndarray) -> np.ndarray:
        return 255 - image

    system.add_custom_filter("invert", invert_colors)

    # Einzelnutzung
    image = cv2.imread("example.jpg")
    filtered_image = system.apply_filter(image, "invert")
    cv2.imwrite("output.jpg", filtered_image)

    # Batch-Verarbeitung
    image_paths = ["example1.jpg", "example2.jpg"]
    operations = [
        lambda img: system.apply_filter(img, "grayscale"),
        lambda img: system.resize(img, (200, 200))
    ]
    system.batch_process(image_paths, "output_folder", operations)
