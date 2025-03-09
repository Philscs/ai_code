import os
import mmap

class PagewiseIterator:
    def __init__(self, filepath, page_size=4096):
        self.filepath = os.path.abspath(filepath)
        self.page_size = page_size
        self.file_size = os.path.getsize(self.filepath)
        self.num_pages = (self.file_size + self.page_size - 1) // self.page_size
        self.current_page = 0
        self.file = None
        self.mmap = None

    def __iter__(self):
        return self

    def __next__(self):
        if self.current_page >= self.num_pages:
            if self.mmap:
                self.mmap.close()
            raise StopIteration

        if not self.file:
            self.file = open(self.filepath, "rb")
            self.mmap = mmap.mmap(self.file.fileno(), 0, access=mmap.ACCESS_READ)

        start = self.current_page * self.page_size
        end = min(start + self.page_size, self.file_size)
        self.current_page += 1

        return self.mmap[start:end]

    def close(self):
        if self.mmap:
            self.mmap.close()
        if self.file:
            self.file.close()

# Example usage
iterator = PagewiseIterator("/path/to/large_dataset.txt", page_size=4096)
for page in iterator:
    # Process each page of data
    print(page)

# Don't forget to close the iterator when done
iterator.close()
