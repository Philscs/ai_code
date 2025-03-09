import os
import cv2
import numpy as np
from PIL import Image
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import accuracy_score

class Bildverarbeitung:
    def __init__(self):
        self.image_path = ""
        self.exif_data = {}
        self.watermark_image = None

    def bereinige_exif_daten(self, image_path):
        """
        Läd EXIF-Daten aus einem Bild und bereinigt sie.
        """
        try:
            img = cv2.imread(image_path)
            exif_data = cv2.getExif(img)
            self.exif_data = {k: v for k, v in exif_data.items() if isinstance(v, str)}
        except Exception as e:
            print(f"Fehler beim Laden der EXIF-Daten: {e}")

    def hinzufuege_watermark(self, watermark_image_path):
        """
        Hinzufügt ein Wasserzeichen zu einem Bild.
        """
        try:
            self.watermark_image = cv2.imread(watermark_image_path)
        except Exception as e:
            print(f"Fehler beim Laden des Watermark-Bildes: {e}")

    def kategorisiere_bilder(self):
        """
        Kategorisiert Bilder basierend auf Machine Learning-Algorithmen.
        """
        try:
            # Bild-Dateien
            bild_dateien = os.listdir("bilder")
            # Filterung ausgewählter Dateitypen
            image_extensions = ['.jpg', '.jpeg', '.png']
            filtered_image_files = [file for file in bild_dateien if 
os.path.splitext(file)[1].lower() in image_extensions]
            images = []
            labels = []

            for i, file in enumerate(filtered_image_files):
                img_path = f"bilder/{file}"
                # Lade Bild und EXIF-Daten
                self.bereinige_exif_daten(img_path)
                # Rausnimmt das Wasserzeichen
                watermark_image = self.watermark_image if self.watermark_image is not None else cv2.imread("watermark.jpg")
                img_without_watermark = cv2.addWeighted(img, 1.0, watermark_image, -0.5, 0)
                # Ermittlung der Category durch Machine Learning
                category = self.kategorisiere_bild(img_without_watermark)

                images.append(img_without_watermark)
                labels.append(category)

            # Trainierung des Modells mit Bildern und ihren entsprechenden Kategorien
            X_train, X_test, y_train, y_test = train_test_split(images, labels, test_size=0.2, random_state=42)
            model = RandomForestClassifier()
            model.fit(X_train, y_train)

            # Prüfung des Modells mit Bildern und ihren entsprechenden Kategorien
            y_pred = model.predict(X_test)
            accuracy = accuracy_score(y_test, y_pred)
            print(f"Genauigkeit des Modells: {accuracy:.2f}%")

        except Exception as e:
            print(f"Fehler beim Kategorisieren der Bilder: {e}")

    def categorisiere_bild(self, img):
        """
        Ermittelt die Category eines Bildes basierend auf Machine Learning.
        """
        try:
            # Hier können Sie ein eigener Algorithmus für die Kategorisierung implementieren
            # Für Demonstrationzwecke wird hier eine einfache Anwendung mit 3 Klassen verwendet
            if np.mean(img) < 128:  # Schattengrau
                return "Schatten"
            elif np.mean(img) < 170:  # Mittelgrau
                return "Mittelgrund"
            else:
                return "Hintergrund"

        except Exception as e:
            print(f"Fehler beim Ermitteln der Category: {e}")