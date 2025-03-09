from hdfs import InsecureClient

# Verbindung zum HDFS-Cluster herstellen
client = InsecureClient('http://localhost:50070', user='hadoop')

# Datei hochladen
client.upload('/path/to/local/file.txt', '/path/to/hdfs/file.txt')

# Datei herunterladen
client.download('/path/to/hdfs/file.txt', '/path/to/local/file.txt')

# Datei löschen
client.delete('/path/to/hdfs/file.txt')

# Verzeichnis erstellen
client.makedirs('/path/to/hdfs/directory')

# Verzeichnisinhalt auflisten
contents = client.list('/path/to/hdfs/directory')
for content in contents:
    print(content)
