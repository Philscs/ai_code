import os
import mmap
from pathlib import Path
from typing import Iterator, Optional, Union

class PagedDatasetIterator:
    """
    Ein sicherer Iterator für die seitenweise Verarbeitung großer Datensätze mit Memory-Mapping.
    Implementiert Schutz gegen Directory Traversal und resource management.
    """
    
    def __init__(
        self,
        file_path: Union[str, Path],
        page_size: int = 4096,
        base_dir: Optional[str] = None
    ):
        """
        Initialisiert den Iterator mit Sicherheitsprüfungen.
        
        Args:
            file_path: Pfad zur Datei
            page_size: Größe jeder Page in Bytes (Standard: 4096)
            base_dir: Optional. Basis-Verzeichnis für Sicherheitsprüfungen
        
        Raises:
            ValueError: Bei ungültigen Parametern
            SecurityError: Bei Sicherheitsverletzungen
        """
        self.page_size = page_size
        if page_size <= 0:
            raise ValueError("Page size muss positiv sein")
            
        # Sicherheitscheck: Normalisiere Pfad und prüfe auf Directory Traversal
        self.file_path = Path(file_path).resolve()
        if base_dir:
            base_path = Path(base_dir).resolve()
            if not str(self.file_path).startswith(str(base_path)):
                raise SecurityError("Zugriff außerhalb des erlaubten Verzeichnisses")
                
        if not self.file_path.is_file():
            raise ValueError(f"Datei nicht gefunden: {self.file_path}")
            
        self.file_size = os.path.getsize(self.file_path)
        self._current_position = 0
        self._mm = None
        self._file = None

    def __enter__(self):
        """Context Manager Entry"""
        self._file = open(self.file_path, 'rb')
        self._mm = mmap.mmap(
            self._file.fileno(),
            0,  # Ganze Datei
            access=mmap.ACCESS_READ
        )
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context Manager Exit mit sicherem Resource Cleanup"""
        if self._mm:
            self._mm.close()
        if self._file:
            self._file.close()

    def __iter__(self) -> Iterator[bytes]:
        """Iterator Interface"""
        return self

    def __next__(self) -> bytes:
        """
        Gibt die nächste Page zurück.
        
        Returns:
            bytes: Nächster Datenblock
            
        Raises:
            StopIteration: Am Ende der Datei
        """
        if self._current_position >= self.file_size:
            raise StopIteration
            
        # Berechne tatsächliche Page-Größe (kann am Ende kleiner sein)
        remaining = self.file_size - self._current_position
        current_page_size = min(self.page_size, remaining)
        
        # Lese Page mit Memory-Mapping
        data = self._mm[self._current_position:self._current_position + current_page_size]
        self._current_position += current_page_size
        
        return data

    def seek(self, position: int) -> None:
        """
        Setzt Position im Iterator.
        
        Args:
            position: Neue Position in Bytes
            
        Raises:
            ValueError: Bei ungültiger Position
        """
        if not 0 <= position <= self.file_size:
            raise ValueError("Position außerhalb der Dateigrenzen")
        self._current_position = position

class SecurityError(Exception):
    """Custom Exception für Sicherheitsverletzungen"""
    pass


# Beispiel Verwendung:
if __name__ == "__main__":
    try:
        # Sichere Verwendung mit Context Manager
        with PagedDatasetIterator("large_dataset.bin", page_size=8192) as iterator:
            # Verarbeite Seiten
            for page in iterator:
                # Verarbeite page bytes...
                pass
                
    except SecurityError as e:
        print(f"Sicherheitsfehler: {e}")
    except Exception as e:
        print(f"Fehler bei der Verarbeitung: {e}")