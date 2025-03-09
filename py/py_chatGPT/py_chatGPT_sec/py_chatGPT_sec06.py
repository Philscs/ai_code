from PIL import Image, ImageDraw, ImageFont
from io import BytesIO
import piexif
import os
import numpy as np
from sklearn.cluster import KMeans
import logging

# Logging einrichten
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

class SecureImageProcessor:
    def __init__(self):
        pass

    @staticmethod
    def remove_exif(image_path, output_path):
        """Entfernt die EXIF-Daten aus einem Bild."""
        try:
            image = Image.open(image_path)
            data = list(image.getdata())
            clean_image = Image.new(image.mode, image.size)
            clean_image.putdata(data)
            clean_image.save(output_path)
            logging.info(f"EXIF-Daten von {image_path} erfolgreich entfernt.")
        except Exception as e:
            logging.error(f"Fehler beim Entfernen der EXIF-Daten: {e}")

    @staticmethod
    def add_watermark(image_path, output_path, watermark_text, position=(10, 10), font_size=20):
        """Fügt einem Bild ein Wasserzeichen hinzu."""
        try:
            image = Image.open(image_path)
            drawable = ImageDraw.Draw(image)
            font = ImageFont.load_default() if not os.path.exists("arial.ttf") else ImageFont.truetype("arial.ttf", font_size)
            drawable.text(position, watermark_text, fill=(255, 255, 255), font=font)
            image.save(output_path)
            logging.info(f"Wasserzeichen zu {image_path} hinzugefügt.")
        except Exception as e:
            logging.error(f"Fehler beim Hinzufügen des Wasserzeichens: {e}")

    @staticmethod
    def classify_image(image_path, model=None):
        """Kategorisiert ein Bild basierend auf Machine Learning-Algorithmen."""
        try:
            image = Image.open(image_path).convert('RGB')
            image_resized = image.resize((128, 128))  # Standardgröße für Modell
            image_array = np.array(image_resized).reshape(-1, 3)

            # Dummy-Modell: K-Means für Farbgruppen
            kmeans = KMeans(n_clusters=3, random_state=0)
            kmeans.fit(image_array)
            dominant_color = kmeans.cluster_centers_[0]
            logging.info(f"Dominante Farbe: {dominant_color}")
            return dominant_color
        except Exception as e:
            logging.error(f"Fehler bei der Bildklassifizierung: {e}")

    @staticmethod
    def validate_image(image_path):
        """Überprüft die Bilddatei auf mögliche Bedrohungen (z. B. Pixel-Flood-Attacken)."""
        try:
            image = Image.open(image_path)
            max_size = 1920 * 1080  # Maximale akzeptierte Pixelanzahl
            pixel_count = image.size[0] * image.size[1]
            
            if pixel_count > max_size:
                raise ValueError("Bildgröße überschreitet die maximal zulässige Größe.")
            logging.info(f"Bildvalidierung für {image_path} erfolgreich.")
        except Exception as e:
            logging.error(f"Fehler bei der Bildvalidierung: {e}")

if __name__ == "__main__":
    processor = SecureImageProcessor()

    # Beispielaufrufe
    processor.remove_exif("example.jpg", "clean_example.jpg")
    processor.add_watermark("example.jpg", "watermarked_example.jpg", "Mein Wasserzeichen")
    dominant_color = processor.classify_image("example.jpg")
    processor.validate_image("example.jpg")
