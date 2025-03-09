import os
import mmap
from typing import Generator

class PageIterator:
    def __init__(self, file_path: str):
        self.file_path = file_path
        self.page_size = 4096  # Standardseitengröße für Dateien

    def _get_page_mapping(self) -> tuple:
        """
        Lese die Datei und generiere eine Mapping für jede Seite.
        
        :return: Eine Paarung von Seiten-Index und Dateimapping
        """
        with open(self.file_path, 'r+b') as file:
            mapping = mmap.mmap(file.fileno(), 0, access=mmap.ACCESS_READ)
            page_mapping = []
            for i in range(0, int(mapping.size()) // self.page_size):
                page_mapping.append((i * self.page_size, mapping[i * self.page_size:i * 
self.page_size + self.page_size]))
            return page_mapping

    def __iter__(self) -> Generator:
        """
        Iteriere über die Datei.
        
        :yield: Ein Paar von Seiten-Index und -Inhalt
        """
        for page_mapping in self._get_page_mapping():
            yield page_mapping

class SafeFileIO:
    def __init__(self, file_path: str):
        self.file_path = file_path
        self.lock_file = f"{self.file_path}.lock"

    def _acquire_lock(self) -> None:
        """
        Schließe den Dateilock.
        
        :raises: PermissionError, wenn es nicht möglich ist, eine neue Datei zu schreiben
        """
        while os.path.exists(self.lock_file):
            raise PermissionError(f"Konnte Datei '{self.file_path}' nicht schreiben.")
        with open(self.lock_file, 'w') as file:
            pass

    def _release_lock(self) -> None:
        """
        Löse den Dateilock.
        
        :raises: FileNotFoundError, wenn es keine Datei gibt
        """
        if not os.path.exists(self.lock_file):
            raise FileNotFoundError(f"Datei '{self.file_path}' nicht gefunden.")
        os.remove(self.lock_file)

    def read_file(self, file_path: str = None) -> bytes:
        """
        Lies die Datei und speichert sie in den Speicher.
        
        :param file_path: Optionaler Pfad zur Datei
        :return: Die Inhalt der Datei als Byte-Array
        """
        self._acquire_lock()
        try:
            if file_path is None:
                with open(self.file_path, 'rb') as file:
                    return file.read()
            else:
                with open(file_path, 'rb') as file:
                    return file.read()
        finally:
            self._release_lock()

class PageReader(SafeFileIO):
    def __init__(self, file_path: str):
        super().__init__(file_path)
        self.iterator = PageIterator(self.file_path)

    def read_page(self, page_index: int) -> bytes:
        """
        Lies eine bestimmte Seite aus der Datei.
        
        :param page_index: Der Index der Seite
        :return: Die Inhalt der Seite als Byte-Array
        """
        for page_mapping in self.iterator:
            if page_mapping[0] == page_index:
                return page_mapping[1]
        raise ValueError(f"Seite {page_index} nicht gefunden.")

if __name__ == "__main__":
    file_path = "test.txt"
    reader = PageReader(file_path)
    page_content = reader.read_page(0)
    print(page_content)
