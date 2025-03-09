import os
import json
import socket
import threading
import shutil
from typing import List, Dict
from dataclasses import dataclass
from datetime import datetime
import random
from pathlib import Path

@dataclass
class FileMetadata:
    filename: str
    size: int
    replicas: List[str]  # Liste der Nodes, die Kopien haben
    last_modified: datetime
    checksum: str

class StorageNode:
    def __init__(self, node_id: str, base_path: str, port: int):
        self.node_id = node_id
        self.base_path = Path(base_path)
        self.base_path.mkdir(exist_ok=True)
        self.port = port
        self.metadata: Dict[str, FileMetadata] = {}
        self.peers: List[str] = []
        self.load = 0
        self._start_server()

    def _start_server(self):
        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server.bind(('localhost', self.port))
        self.server.listen(5)
        
        # Start Server in eigenem Thread
        thread = threading.Thread(target=self._handle_connections)
        thread.daemon = True
        thread.start()

    def _handle_connections(self):
        while True:
            client, addr = self.server.accept()
            thread = threading.Thread(target=self._handle_client, args=(client,))
            thread.daemon = True
            thread.start()

    def _handle_client(self, client):
        try:
            data = client.recv(1024).decode()
            command = json.loads(data)
            
            if command['type'] == 'store':
                self._handle_store(command, client)
            elif command['type'] == 'retrieve':
                self._handle_retrieve(command, client)
            elif command['type'] == 'replicate':
                self._handle_replicate(command, client)
        finally:
            client.close()

    def store_file(self, filename: str, data: bytes):
        # Berechne Checksum
        checksum = self._calculate_checksum(data)
        
        # Speichere Datei lokal
        file_path = self.base_path / filename
        with open(file_path, 'wb') as f:
            f.write(data)
        
        # Aktualisiere Metadata
        metadata = FileMetadata(
            filename=filename,
            size=len(data),
            replicas=[self.node_id],
            last_modified=datetime.now(),
            checksum=checksum
        )
        self.metadata[filename] = metadata
        
        # Repliziere auf andere Nodes
        self._replicate_file(filename, data)
        
        return metadata

    def _replicate_file(self, filename: str, data: bytes):
        # Wähle zufällige Peers für Replikation (hier: 2 Replicas)
        replicas = random.sample(self.peers, min(2, len(self.peers)))
        
        for peer in replicas:
            try:
                # Verbinde mit Peer
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                port = self._get_peer_port(peer)
                s.connect(('localhost', port))
                
                # Sende Replikationsbefehl
                command = {
                    'type': 'replicate',
                    'filename': filename,
                    'data': data.hex()  # Konvertiere bytes zu string für JSON
                }
                s.send(json.dumps(command).encode())
                
                # Update Metadata
                self.metadata[filename].replicas.append(peer)
                
            except Exception as e:
                print(f"Replikation zu {peer} fehlgeschlagen: {e}")
            finally:
                s.close()

    def retrieve_file(self, filename: str) -> bytes:
        if filename not in self.metadata:
            raise FileNotFoundError(f"Datei {filename} nicht gefunden")
            
        file_path = self.base_path / filename
        if not file_path.exists():
            # Versuche Datei von Replica zu holen
            return self._retrieve_from_replica(filename)
            
        with open(file_path, 'rb') as f:
            data = f.read()
            
        # Verifiziere Checksum
        if self._calculate_checksum(data) != self.metadata[filename].checksum:
            # Datei ist korrupt, hole von Replica
            return self._retrieve_from_replica(filename)
            
        return data

    def _retrieve_from_replica(self, filename: str) -> bytes:
        for replica in self.metadata[filename].replicas:
            if replica != self.node_id:
                try:
                    # Verbinde mit Replica
                    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    port = self._get_peer_port(replica)
                    s.connect(('localhost', port))
                    
                    # Sende Retrieve-Befehl
                    command = {
                        'type': 'retrieve',
                        'filename': filename
                    }
                    s.send(json.dumps(command).encode())
                    
                    # Empfange Datei
                    response = s.recv(1024*1024)  # Max 1MB
                    return response
                    
                except Exception as e:
                    print(f"Retrieve von {replica} fehlgeschlagen: {e}")
                finally:
                    s.close()
                    
        raise Exception(f"Keine verfügbare Replica für {filename}")

    def _calculate_checksum(self, data: bytes) -> str:
        import hashlib
        return hashlib.md5(data).hexdigest()

    def _get_peer_port(self, peer_id: str) -> int:
        # In echter Implementierung: Lookup in verteilter Registry
        # Hier vereinfacht: Berechne Port aus peer_id
        return int(peer_id) + 8000

class FileSystemManager:
    def __init__(self):
        self.nodes: List[StorageNode] = []
        
    def add_node(self, node: StorageNode):
        self.nodes.append(node)
        # Update Peer-Listen aller Nodes
        for n in self.nodes:
            n.peers = [node.node_id for node in self.nodes if node != n]
            
    def store_file(self, filename: str, data: bytes):
        # Wähle Node mit geringster Last
        node = min(self.nodes, key=lambda x: x.load)
        node.load += 1  # Simple Lastmetrik
        return node.store_file(filename, data)
        
    def retrieve_file(self, filename: str) -> bytes:
        # Suche Node mit Datei
        for node in self.nodes:
            if filename in node.metadata:
                return node.retrieve_file(filename)
        raise FileNotFoundError(f"Datei {filename} nicht gefunden")

# Beispielnutzung:
if __name__ == "__main__":
    # Erstelle FileSystemManager
    fs_manager = FileSystemManager()
    
    # Erstelle einige Storage Nodes
    for i in range(3):
        node = StorageNode(
            node_id=str(i),
            base_path=f"./storage_node_{i}",
            port=8000 + i
        )
        fs_manager.add_node(node)
    
    # Beispiel: Speichere eine Datei
    test_data = b"Hello, World!"
    metadata = fs_manager.store_file("test.txt", test_data)
    
    # Beispiel: Lese Datei
    retrieved_data = fs_manager.retrieve_file("test.txt")
    print(f"Gespeicherte Daten: {retrieved_data.decode()}")