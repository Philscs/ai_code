import os
import mmap

class PagewiseDatasetIterator:
    def __init__(self, file_path, page_size, encoding='utf-8'):
        """
        Initialisiert den PagewiseDatasetIterator.

        :param file_path: Pfad zur Datei
        :param page_size: Anzahl der Zeilen pro Seite
        :param encoding: Dateikodierung (Standard: 'utf-8')
        """
        self.file_path = file_path
        self.page_size = page_size
        self.encoding = encoding

        # Schutz gegen Directory Traversal
        self._validate_file_path()

        # Datei öffnen
        self.file = open(self.file_path, 'r', encoding=self.encoding)
        self.mmap_file = mmap.mmap(self.file.fileno(), 0, access=mmap.ACCESS_READ)
        self.position = 0

    def _validate_file_path(self):
        """Prüft, ob der Dateipfad sicher ist."""
        base_dir = os.path.abspath(os.getcwd())
        target_path = os.path.abspath(self.file_path)
        if not target_path.startswith(base_dir):
            raise ValueError("Unsicherer Dateipfad: Directory Traversal erkannt.")

    def __iter__(self):
        """Rückgabe des Iterators."""
        return self

    def __next__(self):
        """Liest die nächste Seite aus der Datei."""
        if self.position >= len(self.mmap_file):
            self.close()  # Ressourcen freigeben
            raise StopIteration

        lines = []
        line_count = 0

        while line_count < self.page_size and self.position < len(self.mmap_file):
            start_pos = self.position
            end_pos = self.mmap_file.find(b'\n', start_pos)

            if end_pos == -1:  # Ende der Datei erreicht
                end_pos = len(self.mmap_file)

            line = self.mmap_file[start_pos:end_pos].decode(self.encoding)
            lines.append(line)
            line_count += 1

            self.position = end_pos + 1  # Position hinter das Zeilenende setzen

        return lines

    def close(self):
        """Schließt die Datei und gibt Ressourcen frei."""
        if self.mmap_file:
            self.mmap_file.close()
        if self.file:
            self.file.close()

    def __del__(self):
        """Stellt sicher, dass Ressourcen freigegeben werden."""
        self.close()

# Beispielnutzung
if __name__ == "__main__":
    file_path = "große_datei.txt"
    page_size = 100

    try:
        iterator = PagewiseDatasetIterator(file_path, page_size)
        for page in iterator:
            print("Neue Seite:")
            for line in page:
                print(line)
    except ValueError as e:
        print(f"Fehler: {e}")
    finally:
        iterator.close()
