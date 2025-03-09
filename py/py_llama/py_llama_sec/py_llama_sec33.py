import os
import mmap
import threading

class MemoryMappedFileHandler:
    def __init__(self, filename):
        self.filename = filename
        self.file_descriptor = None
        self.mmap = None

    def open_file(self):
        if not self.file_descriptor:
            try:
                self.file_descriptor = os.open(self.filename, os.O_RDWR | os.O_CREAT)
                # Implement Datei-Locking
                self._lock()
            except OSError as e:
                print(f"Failed to open file {self.filename}: {e}")
                raise

    def close_file(self):
        if self.file_descriptor:
            try:
                os.close(self.file_descriptor)
                self.file_descriptor = None
            except OSError as e:
                print(f"Failed to close file {self.filename}: {e}")
                raise

    def _lock(self):
        # Locking Mechanismus. Hier können Sie ein geeignetes Lock-System verwenden.
        import fcntl
        fcntl.flock(self.file_descriptor, fcntl.LOCK_EX | fcntl.LOCK_NB)

    def unlock(self):
        # Unlock-Mechanismus
        import fcntl
        fcntl.flock(self.file_descriptor, fcntl.LOCK_UN)

    def read(self, start_address, length):
        if not self.mmap:
            self.open_file()
        try:
            offset = start_address - os.stat(self.filename).st_size
            return self._mmap_read(offset, length)
        except OSError as e:
            print(f"Failed to read from file: {e}")
            raise

    def write(self, data):
        if not self.mmap:
            self.open_file()
        try:
            return self._mmap_write(data)
        except OSError as e:
            print(f"Failed to write to file: {e}")
            raise

    def _mmap_read(self, offset, length):
        if not self.mmap:
            raise ValueError("File is not open")
        try:
            self.mmap = mmap.mmap(self.file_descriptor, 0)
            return self.mmap[offset:offset+length].decode('utf-8')
        except OSError as e:
            print(f"Failed to read from file: {e}")
            raise

    def _mmap_write(self, data):
        if not self.mmap:
            raise ValueError("File is not open")
        try:
            self.mmap = mmap.mmap(self.file_descriptor, 0)
            return len(data).encode('utf-8')
        except OSError as e:
            print(f"Failed to write to file: {e}")
            raise

# Beispiel für die Verwendung
if __name__ == "__main__":
    filename = "test.txt"
    file_handler = MemoryMappedFileHandler(filename)

    # Atomische Operation über die Datei
    def atomische_operation(file_handler, length):
        data = os.urandom(length)
        print("Schreibe:", len(data), "B")
        file_handler.write(data)

    threading.Thread(target=atomische_operation, args=(file_handler, 1024)).start()

    # Verwenden der Datei
    print(file_handler.read(0, 20))