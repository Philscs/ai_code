import os
import asyncio
import sqlite3

# Konfiguration
VERzeichnis = '/path/to/directory'
FILTER_Criteria = ['.*\.txt', '.*\.pdf']  # Beispiel-Filter-Kriterien
DB_FILE = 'crawler.db'

# Funktion, um Dateien zu filtern
def filter_file(file_path):
    for criterion in FILTER_Criteria:
        if os.path.match(criterion, file_path):
            return True
    return False

# Funktion, um rekursiv durchsuchte Verzeichnisse zu speichern
async def crawl_directory(directory, path=None):
    if not path:
        path = directory

    # Filter Dateien in Verzeichnis
    for file_name in os.listdir(path):
        file_path = os.path.join(path, file_name)
        
        # Überprüfung auf Symbolic Link-Attacken
        if os.path.islink(file_path):
            continue
        
        # Überprüfung, ob der Ordner sicher ist
        safe_directory = '/safe/ordner'
        if path.startswith(safe_directory):
            continue

        if os.path.isfile(file_path):
            # Datei filtern und speichern in DB
            file_name = os.path.basename(file_path)
            if filter_file(file_name):
                conn = sqlite3.connect(DB_FILE)
                cursor = conn.cursor()
                
                query = "INSERT INTO files (path, name) VALUES (?, ?)"
                cursor.execute(query, (file_path, file_name))
                conn.commit()
                conn.close()

        else:
            # Unterverzeichnis durchsuchen
            await crawl_directory(file_path, path)

# Hauptprogrammfunktion
async def main():
    # Verbindung zur SQLite-Datenbank herstellen
    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()

    # Tabelle erstellen
    query = "CREATE TABLE IF NOT EXISTS files (id INTEGER PRIMARY KEY AUTOINCREMENT, path TEXT, name TEXT)"
    cursor.execute(query)

    # Rekursive Verzeichnisse durchsuchen und filtern
    await crawl_directory(VERzeichnetnis)

    conn.close()

# Ausführen des Programms
asyncio.run(main())