import os
import hashlib
import zlib
import json
from datetime import datetime
from typing import Dict, List, Optional, Set
from dataclasses import dataclass
from collections import defaultdict

@dataclass
class Commit:
    id: str
    message: str
    timestamp: str
    parent_ids: List[str]
    tree_id: str
    author: str

@dataclass
class Branch:
    name: str
    commit_id: str

class VersionControl:
    def __init__(self, root_path: str):
        self.root_path = root_path
        self.objects_path = os.path.join(root_path, ".vcs/objects")
        self.refs_path = os.path.join(root_path, ".vcs/refs")
        self.head_path = os.path.join(root_path, ".vcs/HEAD")
        
        # Initialisiere Verzeichnisstruktur
        os.makedirs(self.objects_path, exist_ok=True)
        os.makedirs(self.refs_path, exist_ok=True)
        
        if not os.path.exists(self.head_path):
            with open(self.head_path, "w") as f:
                f.write("ref: refs/heads/main")

    def hash_object(self, data: bytes, obj_type: str) -> str:
        """Erstellt einen Hash für ein Objekt und speichert es."""
        header = f"{obj_type} {len(data)}".encode()
        full_data = header + b"\x00" + data
        sha1 = hashlib.sha1(full_data).hexdigest()
        
        # Komprimiere und speichere das Objekt
        compressed = zlib.compress(full_data)
        path = os.path.join(self.objects_path, sha1[:2], sha1[2:])
        os.makedirs(os.path.dirname(path), exist_ok=True)
        
        with open(path, "wb") as f:
            f.write(compressed)
        
        return sha1

    def read_object(self, sha1: str) -> tuple:
        """Liest ein Objekt aus dem Objektspeicher."""
        path = os.path.join(self.objects_path, sha1[:2], sha1[2:])
        
        with open(path, "rb") as f:
            data = zlib.decompress(f.read())
        
        # Parse Header
        null_index = data.index(b"\x00")
        header = data[:null_index].decode()
        obj_type, size = header.split()
        
        # Extrahiere Objekt-Daten
        content = data[null_index + 1:]
        return obj_type, content

    def create_blob(self, file_path: str) -> str:
        """Erstellt einen Blob aus einer Datei."""
        with open(file_path, "rb") as f:
            content = f.read()
        return self.hash_object(content, "blob")

    def create_tree(self, directory: str) -> str:
        """Erstellt einen Tree aus einem Verzeichnis."""
        entries = []
        
        for name in os.listdir(directory):
            path = os.path.join(directory, name)
            if os.path.isfile(path):
                # Für Dateien: erstelle Blob
                mode = "100644"
                obj_id = self.create_blob(path)
                entries.append((mode, "blob", obj_id, name))
            elif os.path.isdir(path) and not name.startswith('.'):
                # Für Verzeichnisse: erstelle Sub-Tree
                mode = "040000"
                obj_id = self.create_tree(path)
                entries.append((mode, "tree", obj_id, name))
        
        # Sortiere Einträge und erstelle Tree-Inhalt
        entries.sort(key=lambda x: x[3])
        tree_content = "\n".join(f"{mode} {type} {id}\t{name}" 
                               for mode, type, id, name in entries)
        return self.hash_object(tree_content.encode(), "tree")

    def create_commit(self, tree_id: str, message: str, parent_ids: List[str]) -> str:
        """Erstellt einen neuen Commit."""
        commit_data = {
            "tree": tree_id,
            "parent": parent_ids,
            "author": "User <user@example.com>",
            "timestamp": datetime.now().isoformat(),
            "message": message
        }
        
        commit_content = json.dumps(commit_data, indent=2).encode()
        return self.hash_object(commit_content, "commit")

    def get_branch_commit(self, branch_name: str) -> Optional[str]:
        """Gibt die Commit-ID eines Branches zurück."""
        branch_path = os.path.join(self.refs_path, "heads", branch_name)
        if os.path.exists(branch_path):
            with open(branch_path, "r") as f:
                return f.read().strip()
        return None

    def create_branch(self, name: str, commit_id: str):
        """Erstellt einen neuen Branch."""
        branch_path = os.path.join(self.refs_path, "heads", name)
        os.makedirs(os.path.dirname(branch_path), exist_ok=True)
        with open(branch_path, "w") as f:
            f.write(commit_id)

    def merge_branches(self, source_branch: str, target_branch: str) -> Optional[str]:
        """
        Führt zwei Branches zusammen und erstellt einen Merge-Commit.
        Returns: Merge-Commit-ID oder None bei Konflikten
        """
        source_commit = self.get_branch_commit(source_branch)
        target_commit = self.get_branch_commit(target_branch)
        
        if not source_commit or not target_commit:
            raise ValueError("Branch nicht gefunden")
        
        # Finde gemeinsamen Vorfahren
        base_commit = self.find_merge_base(source_commit, target_commit)
        if not base_commit:
            raise ValueError("Kein gemeinsamer Vorfahre gefunden")
        
        # Führe Three-Way-Merge durch
        merged_tree = self.three_way_merge(
            self.get_commit_tree(base_commit),
            self.get_commit_tree(source_commit),
            self.get_commit_tree(target_commit)
        )
        
        if merged_tree:
            # Erstelle Merge-Commit
            message = f"Merge branch '{source_branch}' into '{target_branch}'"
            merge_commit = self.create_commit(
                merged_tree,
                message,
                [target_commit, source_commit]
            )
            return merge_commit
        
        return None  # Konflikt

    def get_commit_tree(self, commit_id: str) -> str:
        """Holt die Tree-ID eines Commits."""
        _, commit_data = self.read_object(commit_id)
        commit_info = json.loads(commit_data.decode())
        return commit_info["tree"]

    def find_merge_base(self, commit1: str, commit2: str) -> Optional[str]:
        """Findet den letzten gemeinsamen Vorfahren zweier Commits."""
        commits1 = self.get_commit_history(commit1)
        commits2 = self.get_commit_history(commit2)
        
        # Finde erste Überschneidung in den Historien
        for commit in commits1:
            if commit in commits2:
                return commit
        
        return None

    def get_commit_history(self, start_commit: str) -> Set[str]:
        """Gibt alle Vorfahren eines Commits zurück."""
        history = set()
        queue = [start_commit]
        
        while queue:
            commit_id = queue.pop(0)
            if commit_id in history:
                continue
                
            history.add(commit_id)
            
            # Füge Eltern-Commits zur Queue hinzu
            _, commit_data = self.read_object(commit_id)
            commit_info = json.loads(commit_data.decode())
            queue.extend(commit_info["parent"])
        
        return history

    def three_way_merge(self, base_tree: str, source_tree: str, target_tree: str) -> Optional[str]:
        """
        Führt einen Three-Way-Merge durch.
        Returns: Merged Tree ID oder None bei Konflikten
        """
        # Hole Tree-Inhalte
        _, base_content = self.read_object(base_tree)
        _, source_content = self.read_object(source_tree)
        _, target_content = self.read_object(target_tree)
        
        # Parse Tree-Einträge
        base_entries = self._parse_tree_content(base_content)
        source_entries = self._parse_tree_content(source_content)
        target_entries = self._parse_tree_content(target_content)
        
        # Merge-Logik implementieren
        merged_entries = {}
        has_conflicts = False
        
        # Verarbeite alle Einträge
        all_paths = set(base_entries.keys()) | set(source_entries.keys()) | set(target_entries.keys())
        
        for path in all_paths:
            base = base_entries.get(path)
            source = source_entries.get(path)
            target = target_entries.get(path)
            
            # Keine Änderungen
            if source == target:
                merged_entries[path] = source or target
            # Nur in einer Branch geändert
            elif source == base:
                merged_entries[path] = target
            elif target == base:
                merged_entries[path] = source
            # Konflikt
            else:
                has_conflicts = True
                break
        
        if has_conflicts:
            return None
            
        # Erstelle neuen Tree aus gemergten Einträgen
        merged_content = self._create_tree_content(merged_entries)
        return self.hash_object(merged_content.encode(), "tree")

    def _parse_tree_content(self, content: bytes) -> Dict[str, tuple]:
        """Parse Tree-Inhalt in ein Dictionary von Pfad zu (mode, type, id)."""
        entries = {}
        for line in content.decode().split("\n"):
            if not line:
                continue
            mode, obj_type, obj_id, name = line.split()
            entries[name] = (mode, obj_type, obj_id)
        return entries

    def _create_tree_content(self, entries: Dict[str, tuple]) -> str:
        """Erstellt Tree-Inhalt aus Dictionary von Einträgen."""
        lines = []
        for name, (mode, obj_type, obj_id) in sorted(entries.items()):
            lines.append(f"{mode} {obj_type} {obj_id}\t{name}")
        return "\n".join(lines)

# Beispiel-Nutzung:
if __name__ == "__main__":
    # Initialisiere VCS in aktuellem Verzeichnis
    vcs = VersionControl(".")
    
    # Erstelle initialen Commit
    tree_id = vcs.create_tree(".")
    initial_commit = vcs.create_commit(tree_id, "Initial commit", [])
    vcs.create_branch("main", initial_commit)
    
    # Erstelle Feature-Branch
    vcs.create_branch("feature", initial_commit)
    
    # Mache Änderungen und committe sie
    # ... (Dateien ändern)
    new_tree_id = vcs.create_tree(".")
    feature_commit = vcs.create_commit(new_tree_id, "Add feature", [initial_commit])
    
    # Update Feature-Branch
    vcs.create_branch("feature", feature_commit)
    
    # Merge Feature in Main
    merge_commit = vcs.merge_branches("feature", "main")
    if merge_commit:
        vcs.create_branch("main", merge_commit)
    else:
        print("Merge-Konflikt aufgetreten!")