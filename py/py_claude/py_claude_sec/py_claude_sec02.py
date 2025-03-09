import asyncio
import os
import pathlib
import sqlite3
from typing import List, Set, Dict, Optional
import logging
from dataclasses import dataclass
from datetime import datetime

@dataclass
class FileInfo:
    path: str
    size: int
    modified: datetime
    file_type: str
    
class SafeCrawler:
    def __init__(self, 
                 root_paths: List[str],
                 excluded_paths: List[str],
                 db_path: str,
                 file_patterns: List[str]):
        self.root_paths = [pathlib.Path(p).resolve() for p in root_paths]
        self.excluded_paths = [pathlib.Path(p).resolve() for p in excluded_paths]
        self.db_path = db_path
        self.file_patterns = file_patterns
        self.visited_links: Set[str] = set()
        
        # Logging konfigurieren
        logging.basicConfig(
            level=logging.INFO,
            format='%(asctime)s - %(levelname)s - %(message)s'
        )
        self.logger = logging.getLogger(__name__)
        
        # Datenbank initialisieren
        self._init_db()

    def _init_db(self) -> None:
        """Initialisiert die SQLite-Datenbank mit dem benötigten Schema."""
        with sqlite3.connect(self.db_path) as conn:
            conn.execute('''
                CREATE TABLE IF NOT EXISTS files (
                    id INTEGER PRIMARY KEY,
                    path TEXT UNIQUE,
                    size INTEGER,
                    modified TIMESTAMP,
                    file_type TEXT,
                    scan_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                )
            ''')

    def _is_path_safe(self, path: pathlib.Path) -> bool:
        """
        Überprüft, ob ein Pfad sicher durchsucht werden kann.
        Verhindert das Durchsuchen von sensiblen Systempfaden und Symlink-Attacks.
        """
        try:
            resolved_path = path.resolve()
            
            # Prüfe auf ausgeschlossene Pfade
            for excluded in self.excluded_paths:
                if resolved_path.is_relative_to(excluded):
                    return False
            
            # Prüfe auf bereits besuchte Symlinks
            if path.is_symlink():
                real_path = str(resolved_path)
                if real_path in self.visited_links:
                    return False
                self.visited_links.add(real_path)
            
            return True
            
        except (RuntimeError, OSError) as e:
            self.logger.warning(f"Fehler beim Überprüfen des Pfads {path}: {e}")
            return False

    async def _process_file(self, path: pathlib.Path) -> Optional[FileInfo]:
        """Verarbeitet eine einzelne Datei und sammelt relevante Informationen."""
        try:
            if not any(path.match(pattern) for pattern in self.file_patterns):
                return None
                
            stats = path.stat()
            return FileInfo(
                path=str(path),
                size=stats.st_size,
                modified=datetime.fromtimestamp(stats.st_mtime),
                file_type=path.suffix
            )
            
        except (OSError, RuntimeError) as e:
            self.logger.error(f"Fehler beim Verarbeiten von {path}: {e}")
            return None

    async def _scan_directory(self, directory: pathlib.Path) -> List[FileInfo]:
        """Durchsucht ein Verzeichnis asynchron nach Dateien."""
        results: List[FileInfo] = []
        
        try:
            for entry in os.scandir(str(directory)):
                path = pathlib.Path(entry.path)
                
                if not self._is_path_safe(path):
                    continue
                    
                if entry.is_file():
                    if file_info := await self._process_file(path):
                        results.append(file_info)
                        
                elif entry.is_dir():
                    results.extend(await self._scan_directory(path))
                    
        except (OSError, RuntimeError) as e:
            self.logger.error(f"Fehler beim Scannen von {directory}: {e}")
            
        return results

    def _save_to_db(self, files: List[FileInfo]) -> None:
        """Speichert die gefundenen Dateien in der Datenbank."""
        with sqlite3.connect(self.db_path) as conn:
            conn.executemany('''
                INSERT OR REPLACE INTO files (path, size, modified, file_type)
                VALUES (?, ?, ?, ?)
            ''', [(f.path, f.size, f.modified, f.file_type) for f in files])

    async def crawl(self) -> None:
        """Hauptmethode zum Starten des Crawling-Prozesses."""
        all_files: List[FileInfo] = []
        
        for root in self.root_paths:
            if not self._is_path_safe(root):
                self.logger.warning(f"Überspringe unsicheren Root-Pfad: {root}")
                continue
                
            self.logger.info(f"Starte Scan von: {root}")
            files = await self._scan_directory(root)
            all_files.extend(files)
            
        self.logger.info(f"Gefundene Dateien: {len(all_files)}")
        self._save_to_db(all_files)

# Beispiel zur Verwendung
async def main():
    crawler = SafeCrawler(
        root_paths=["/path/to/scan"],
        excluded_paths=[
            "/etc",
            "/sys",
            "/proc",
            "/dev"
        ],
        db_path="file_index.db",
        file_patterns=["*.pdf", "*.txt", "*.doc*"]
    )
    
    await crawler.crawl()

if __name__ == "__main__":
    asyncio.run(main())