
import mmap
import os
import fcntl
import mmap
import os
import fcntl
import threading
from contextlib import contextmanager
from typing import Optional, Union, BinaryIO

# Open the file in read-write mode
file_path = "/path/to/large_file.txt"
file = open(file_path, "r+b")

# Get the file size
file_size = os.path.getsize(file_path)

# Create a memory-mapped file handler
mmapped_file = mmap.mmap(file.fileno(), file_size, access=mmap.ACCESS_WRITE)

# Lock the file to prevent other processes from accessing it
fcntl.flock(file.fileno(), fcntl.LOCK_EX)

# Perform atomic operations on the memory-mapped file
# ...

# Unlock the file
fcntl.flock(file.fileno(), fcntl.LOCK_UN)

# Close the file and release the memory-mapped file handler
mmapped_file.close()
file.close()

class MMapHandler:
    """
    Ein Thread-sicherer Memory-mapped File Handler mit Datei-Locking und atomaren Operationen.
    """
    
    def __init__(self, filename: str, size: Optional[int] = None):
        """
        Initialisiert den MMapHandler.
        
        Args:
            filename: Pfad zur Datei
            size: Optionale Größe für neue Dateien in Bytes
        """
        self.filename = filename
        self.lock = threading.Lock()
        self._mmap: Optional[mmap.mmap] = None
        self._file: Optional[BinaryIO] = None
        
        # Erstelle Datei falls nicht vorhanden
        if not os.path.exists(filename):
            if size is None:
                raise ValueError("size muss für neue Dateien angegeben werden")
            with open(filename, "wb") as f:
                f.write(b'\0' * size)
    
    @contextmanager
    def _file_lock(self):
        """Kontextmanager für exklusives Datei-Locking."""
        try:
            # Thread-Lock erwerben
            self.lock.acquire()
            
            # Datei öffnen
            self._file = open(self.filename, "r+b")
            
            # Exklusives Datei-Lock setzen
            fcntl.flock(self._file.fileno(), fcntl.LOCK_EX)
            
            # Memory-Map erstellen
            self._mmap = mmap.mmap(self._file.fileno(), 0)
            
            yield self._mmap
            
        finally:
            # Aufräumen in umgekehrter Reihenfolge
            if self._mmap is not None:
                self._mmap.close()
                self._mmap = None
                
            if self._file is not None:
                fcntl.flock(self._file.fileno(), fcntl.LOCK_UN)
                self._file.close()
                self._file = None
                
            self.lock.release()
    
    def atomic_write(self, offset: int, data: Union[bytes, bytearray]) -> None:
        """
        Führt einen atomaren Schreibvorgang aus.
        
        Args:
            offset: Position in der Datei
            data: Zu schreibende Bytes
        """
        with self._file_lock() as mm:
            mm.seek(offset)
            mm.write(data)
            mm.flush()  # Änderungen auf Platte sichern
    
    def atomic_read(self, offset: int, size: int) -> bytes:
        """
        Führt einen atomaren Lesevorgang aus.
        
        Args:
            offset: Position in der Datei
            size: Anzahl zu lesender Bytes
            
        Returns:
            Gelesene Bytes
        """
        with self._file_lock() as mm:
            mm.seek(offset)
            return mm.read(size)
    
    def get_size(self) -> int:
        """Gibt die Größe der Datei zurück."""
        return os.path.getsize(self.filename)


# Beispielnutzung:
if __name__ == "__main__":
    # Handler für 1MB Datei erstellen
    handler = MMapHandler("test.dat", 1024 * 1024)
    
    # Atomares Schreiben
    handler.atomic_write(0, b"Hello World")
    
    # Atomares Lesen
    data = handler.atomic_read(0, 11)
    print(data.decode())  # Gibt "Hello World" aus
    
    # Mehrere Threads können sicher zugreifen
    def worker(handler, offset):
        handler.atomic_write(offset, b"Thread Data")
    
    threads = []
    for i in range(5):
        t = threading.Thread(target=worker, args=(handler, i * 100))
        threads.append(t)
        t.start()
    
    for t in threads:
        t.join()
