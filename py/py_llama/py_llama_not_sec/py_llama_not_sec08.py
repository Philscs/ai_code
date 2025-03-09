import cv2
import numpy as np

class Bildverarbeitung:
    def __init__(self):
        self.filter = {
            'gauss': self.gauss_filter,
            'blur': self_blur,
            'laplacian': self.laplacian_filter
        }
        self.batch_size = 5

    def gauss_filter(self, bild):
        """
        Durchführung eines Gauss-Filters auf einem Bild.
        
        :param bild: Ein Bild aus einer Datei oder als numpy-array
        :return: Ein Bild mit dem angewendeten Filter
        """
        # Erstellen eines Gaussian-Bildfilters
        gaussian_kernel = np.array([[1/16, 1/8, 1/16], [1/8, 1/4, 1/8], [1/16, 1/8, 1/16]])
        
        # Verwenden der Bildfilterfunktion aus OpenCV
        return cv2.filter2D(bild, -1, gaussian_kernel)

    def blur(self, bild):
        """
        Durchführung eines Blurs-Filters auf einem Bild.
        
        :param bild: Ein Bild aus einer Datei oder als numpy-array
        :return: Ein Bild mit dem angewendeten Filter
        """
        # Verwenden der Bildfilterfunktion aus OpenCV
        return cv2.GaussianBlur(bild, (5, 5), 0)

    def laplacian_filter(self, bild):
        """
        Durchführung eines Laplace-Filter auf einem Bild.
        
        :param bild: Ein Bild aus einer Datei oder als numpy-array
        :return: Ein Bild mit dem angewendeten Filter
        """
        # Verwenden der Bildfilterfunktion aus OpenCV
        return cv2.Laplacian(bild, cv2.CV_8U)

    def batch_verarbeitung(self, bild_liste):
        """
        Durchführung einer Batch-Verarbeitung an einem Liste von Bildern.
        
        :param bild_liste: Eine Liste mit Bildern (numpy-array oder OpenCV-Mat)
        :return: Eine Liste mit den angewendeten Filtern
        """
        result_list = []
        for bild in bild_liste:
            # Durchführung des Filters auf jedes Bild und Addieren zum Ergebnis
            if 'gauss' in self.filter:
                result_list.append(self.gauss_filter(bild))
            elif 'blur' in self.filter:
                result_list.append(self.blur(bild))
            elif 'laplacian' in self.filter:
                result_list.append(self.laplacian_filter(bild))
        return result_list

    def __call__(self, bild):
        """
        Durchführung eines Bildfilters.
        
        :param bild: Ein Bild aus einer Datei oder als numpy-array
        :return: Ein Bild mit dem angewendeten Filter
        """
        # Prüfung, ob ein Filter in der Liste verfügbar ist
        if 'gauss' in self.filter:
            result = self.gauss_filter(bild)
        elif 'blur' in self.filter:
            result = self.blur(bild)
        elif 'laplacian' in self.filter:
            result = self.laplacian_filter(bild)
        
        # Rückgabe des Ergebnisses
        return result

# Erstellung eines Bildverarbeitungsobjekts
verarbeitung = Bildverarbeitung()

# Verwendung des Bildverarbeitungsobjekts
bild_liste = [cv2.imread('bild1.jpg'), cv2.imread('bild2.jpg')]
result_list = verarbeitung.batch_verarbeitung(bild_liste)

for i, result in enumerate(result_list):
    cv2.imshow(f"Bild {i+1}", result)
    cv2.waitKey(0)