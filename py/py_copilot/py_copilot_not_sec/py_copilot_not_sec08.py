import cv2
import numpy as np

class ImageProcessor:
    def __init__(self, image_path):
        self.image = cv2.imread(image_path)

    def apply_filter(self, filter_type):
        if filter_type == 'grayscale':
            self.image = cv2.cvtColor(self.image, cv2.COLOR_BGR2GRAY)
        elif filter_type == 'blur':
            self.image = cv2.GaussianBlur(self.image, (5, 5), 0)
        # Add more filter types here

    def apply_transformation(self, transformation_type):
        if transformation_type == 'rotate':
            rows, cols = self.image.shape[:2]
            M = cv2.getRotationMatrix2D((cols / 2, rows / 2), 90, 1)
            self.image = cv2.warpAffine(self.image, M, (cols, rows))
        elif transformation_type == 'resize':
            self.image = cv2.resize(self.image, (500, 500))
        # Add more transformation types here

    def analyze(self):
        # Example analysis function
        image_mean = np.mean(self.image)
        image_std = np.std(self.image)
        print("Image mean:", image_mean)
        print("Image standard deviation:", image_std)
    def save_image(self, output_path):
        cv2.imwrite(output_path, self.image)

if __name__ == '__main__':
    image_processor = ImageProcessor('/path/to/input/image.jpg')
    image_processor.apply_filter('grayscale')
    image_processor.apply_transformation('rotate')
    image_processor.analyze()
    image_processor.save_image('/path/to/output/image.jpg')
