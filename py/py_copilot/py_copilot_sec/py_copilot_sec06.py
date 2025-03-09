import os
import PIL
from PIL import Image, ImageDraw, ImageFont
from PIL.ExifTags import TAGS
import numpy as np
from typing import Optional, Dict, List, Tuple
import tensorflow as tf
import logging
import hashlib

class SecureImageProcessor:
    """Hauptklasse für sichere Bildverarbeitung mit EXIF-Bereinigung,
    Wasserzeichen und ML-basierter Kategorisierung."""
    
    MAX_IMAGE_DIMENSION = 8000  # Maximale Bildgröße zur Vermeidung von Pixel-Flood
    ALLOWED_FORMATS = {'JPEG', 'PNG', 'BMP', 'TIFF'}
    
    def __init__(self, model_path: Optional[str] = None):
        """
        Initialisiert den Image Processor.
        
        Args:
            model_path: Pfad zum vortrainierten Kategorisierungsmodell
        """
        self.logger = logging.getLogger(__name__)
        self.model = self._load_model(model_path) if model_path else None
        
    def _load_model(self, model_path: str) -> tf.keras.Model:
        """Lädt das ML-Modell für die Bildkategorisierung."""
        try:
            return tf.keras.models.load_model(model_path)
        except Exception as e:
            self.logger.error(f"Fehler beim Laden des Models: {e}")
            raise
            
    def _verify_image(self, image_path: str) -> bool:
        """
        Überprüft, ob ein Bild sicher verarbeitet werden kann.
        
        Args:
            image_path: Pfad zum Eingabebild
            
        Returns:
            bool: True wenn das Bild sicher ist, sonst False
        """
        try:
            with Image.open(image_path) as img:
                # Überprüfe Dateityp
                if img.format not in self.ALLOWED_FORMATS:
                    self.logger.warning(f"Nicht unterstütztes Bildformat: {img.format}")
                    return False
                
                # Überprüfe Bildgröße
                if (img.width > self.MAX_IMAGE_DIMENSION or 
                    img.height > self.MAX_IMAGE_DIMENSION):
                    self.logger.warning("Bild überschreitet maximale Dimensionen")
                    return False
                
                # Validiere Bilddaten
                img.verify()
                return True
                
        except Exception as e:
            self.logger.error(f"Fehler bei der Bildvalidierung: {e}")
            return False
            
    def clean_exif(self, image_path: str, output_path: str) -> bool:
        """
        Entfernt EXIF-Daten aus dem Bild.
        
        Args:
            image_path: Eingabebild
            output_path: Pfad für bereinigtes Bild
            
        Returns:
            bool: True bei erfolgreicher Bereinigung
        """
        if not self._verify_image(image_path):
            return False
            
        try:
            with Image.open(image_path) as img:
                # Erstelle neue Bilddaten ohne EXIF
                data = list(img.getdata())
                clean_img = Image.new(img.mode, img.size)
                clean_img.putdata(data)
                
                clean_img.save(output_path)
                return True
                
        except Exception as e:
            self.logger.error(f"Fehler bei EXIF-Bereinigung: {e}")
            return False
            
    def add_watermark(self, 
                     image_path: str,
                     output_path: str,
                     watermark_text: str,
                     position: Tuple[int, int] = None,
                     font_size: int = 36,
                     opacity: int = 128) -> bool:
        """
        Fügt ein transparentes Wasserzeichen zum Bild hinzu.
        
        Args:
            image_path: Eingabebild
            output_path: Ausgabebild mit Wasserzeichen
            watermark_text: Text des Wasserzeichens
            position: (x,y) Position des Wasserzeichens
            font_size: Schriftgröße
            opacity: Transparenz (0-255)
        """
        if not self._verify_image(image_path):
            return False
            
        try:
            with Image.open(image_path) as img:
                # Erstelle transparente Ebene für Wasserzeichen
                watermark = Image.new('RGBA', img.size, (0,0,0,0))
                draw = ImageDraw.Draw(watermark)
                
                # Lade Font
                try:
                    font = ImageFont.truetype("arial.ttf", font_size)
                except:
                    font = ImageFont.load_default()
                
                # Berechne Position wenn nicht angegeben
                if position is None:
                    text_width = draw.textlength(watermark_text, font=font)
                    text_height = font_size
                    position = (
                        (img.width - text_width) // 2,
                        (img.height - text_height) // 2
                    )
                
                # Zeichne Wasserzeichen
                draw.text(
                    position,
                    watermark_text,
                    font=font,
                    fill=(255,255,255,opacity)
                )
                
                # Kombiniere Bild und Wasserzeichen
                if img.mode != 'RGBA':
                    img = img.convert('RGBA')
                    
                watermarked = Image.alpha_composite(img, watermark)
                watermarked.save(output_path)
                return True
                
        except Exception as e:
            self.logger.error(f"Fehler beim Hinzufügen des Wasserzeichens: {e}")
            return False
            
    def categorize_image(self, image_path: str) -> Optional[str]:
        """
        Kategorisiert ein Bild mittels ML-Modell.
        
        Args:
            image_path: Zu kategorisierendes Bild
            
        Returns:
            Optional[str]: Vorhergesagte Kategorie oder None bei Fehler
        """
        if not self._verify_image(image_path) or self.model is None:
            return None
            
        try:
            with Image.open(image_path) as img:
                # Prepare image for model
                img_array = tf.keras.preprocessing.image.img_to_array(img)
                img_array = tf.expand_dims(img_array, 0)
                
                # Make prediction
                predictions = self.model.predict(img_array)
                return self._decode_predictions(predictions)
                
        except Exception as e:
            self.logger.error(f"Fehler bei der Bildkategorisierung: {e}")
            return None
            
    def _decode_predictions(self, predictions: np.ndarray) -> str:
        """Dekodiert Modellvorhersagen in Kategorie-Labels."""
        # Implementierung abhängig vom verwendeten Modell
        # Hier vereinfachtes Beispiel
        return str(np.argmax(predictions[0]))

# Beispielverwendung:
if __name__ == "__main__":
    processor = SecureImageProcessor()
    
    # Beispiel für Bildverarbeitung
    input_image = "input.jpg"
    
    # EXIF bereinigen
    processor.clean_exif(input_image, "clean.jpg")
    
    # Wasserzeichen hinzufügen
    processor.add_watermark(
        "clean.jpg",
        "watermarked.jpg",
        "Copyright 2024",
        opacity=100
    )
    
    # Bild kategorisieren (benötigt trainiertes Modell)
    if processor.model:
        category = processor.categorize_image("watermarked.jpg")
        print(f"Bildkategorie: {category}")
