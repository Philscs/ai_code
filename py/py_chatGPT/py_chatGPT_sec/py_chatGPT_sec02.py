import os
import asyncio
import aiosqlite
from pathlib import Path
from typing import Callable, List

def is_symlink_secure(base_path: Path, target_path: Path) -> bool:
    """
    Check if a symbolic link points within the allowed directory.
    """
    try:
        base_path = base_path.resolve(strict=True)
        target_path = target_path.resolve(strict=True)
        return target_path.is_relative_to(base_path)
    except Exception:
        return False

async def insert_into_db(db_path: str, entries: List[dict]):
    """Insert file entries into SQLite database."""
    async with aiosqlite.connect(db_path) as db:
        await db.execute("""
            CREATE TABLE IF NOT EXISTS files (
                id INTEGER PRIMARY KEY,
                path TEXT NOT NULL,
                size INTEGER NOT NULL,
                modified_time TEXT NOT NULL
            )
        """)
        await db.executemany("""
            INSERT INTO files (path, size, modified_time) VALUES (?, ?, ?)
        """, [(e['path'], e['size'], e['modified_time']) for e in entries])
        await db.commit()

async def crawl_directory(
    base_path: Path,
    db_path: str,
    filter_func: Callable[[Path], bool],
    excluded_dirs: List[str]
):
    """
    Recursively crawl a directory, filter files, and save results in SQLite database.
    """
    entries = []
    for root, dirs, files in os.walk(base_path):
        root_path = Path(root)

        # Exclude specified directories
        dirs[:] = [d for d in dirs if root_path / d not in excluded_dirs]

        for file in files:
            file_path = root_path / file

            try:
                # Skip symbolic links if not secure
                if file_path.is_symlink() and not is_symlink_secure(base_path, file_path):
                    continue

                if filter_func(file_path):
                    file_stat = file_path.stat()
                    entries.append({
                        "path": str(file_path),
                        "size": file_stat.st_size,
                        "modified_time": file_stat.st_mtime,
                    })
            except (PermissionError, FileNotFoundError):
                continue

    if entries:
        await insert_into_db(db_path, entries)

async def main():
    base_dir = Path("/path/to/scan")
    db_path = "files.db"

    # Define filtering criteria (e.g., only .txt files)
    def filter_func(file_path: Path) -> bool:
        return file_path.suffix == ".txt"

    # Define excluded directories
    excluded_dirs = [Path("/path/to/exclude1"), Path("/path/to/exclude2")]

    # Ensure excluded directories are absolute paths
    excluded_dirs = [d.resolve(strict=True) for d in excluded_dirs]

    await crawl_directory(base_dir, db_path, filter_func, excluded_dirs)

if __name__ == "__main__":
    asyncio.run(main())
