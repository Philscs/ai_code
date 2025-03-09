import os
import mmap
import fcntl
from contextlib import contextmanager

class MemoryMappedFileHandler:
    def __init__(self, filepath, size):
        """Initialize the memory-mapped file handler.

        Args:
            filepath (str): Path to the file to be memory-mapped.
            size (int): Size of the file in bytes. If the file doesn't exist, it will be created with this size.
        """
        self.filepath = filepath
        self.size = size

        # Ensure the file exists and has the correct size
        with open(self.filepath, 'a+b') as f:
            f.truncate(self.size)

        # Open the file and memory-map it
        self.file = open(self.filepath, 'r+b')
        self.mapped_file = mmap.mmap(self.file.fileno(), self.size)

    def __del__(self):
        """Ensure resources are cleaned up on destruction."""
        self.mapped_file.close()
        self.file.close()

    @contextmanager
    def lock(self):
        """Context manager to lock the file for exclusive access."""
        try:
            fcntl.flock(self.file.fileno(), fcntl.LOCK_EX)
            yield
        finally:
            fcntl.flock(self.file.fileno(), fcntl.LOCK_UN)

    def read(self, start, length):
        """Read data from the memory-mapped file atomically.

        Args:
            start (int): Starting position to read from.
            length (int): Number of bytes to read.

        Returns:
            bytes: The data read from the file.
        """
        with self.lock():
            self.mapped_file.seek(start)
            return self.mapped_file.read(length)

    def write(self, start, data):
        """Write data to the memory-mapped file atomically.

        Args:
            start (int): Starting position to write to.
            data (bytes): The data to write.
        """
        if start + len(data) > self.size:
            raise ValueError("Write exceeds file boundaries.")

        with self.lock():
            self.mapped_file.seek(start)
            self.mapped_file.write(data)
            self.mapped_file.flush()

    def close(self):
        """Close the memory-mapped file and associated resources."""
        self.mapped_file.close()
        self.file.close()

# Example usage
if __name__ == "__main__":
    filepath = "example.dat"
    file_size = 1024  # 1 KB file

    handler = MemoryMappedFileHandler(filepath, file_size)

    # Write data atomically
    handler.write(0, b"Hello, world!")

    # Read data atomically
    data = handler.read(0, 13)
    print("Read data:", data.decode())

    # Clean up
    handler.close()
