from typing import Dict, Set, List
import threading
import time
import uuid
from dataclasses import dataclass
from collections import defaultdict

@dataclass
class LockRequest:
    resource_id: str
    process_id: str
    timestamp: float

class DistributedLockManager:
    def __init__(self):
        self.locks: Dict[str, str] = {}  # resource_id -> process_id
        self.waiting: Dict[str, List[LockRequest]] = defaultdict(list)
        self.held_locks: Dict[str, Set[str]] = defaultdict(set)  # process_id -> set of resource_ids
        self.lock = threading.Lock()
        
    def request_lock(self, resource_id: str, process_id: str) -> bool:
        """
        Versucht einen Lock für eine Ressource zu erwerben.
        Returns True wenn der Lock erfolgreich erworben wurde, False wenn warten notwendig ist.
        """
        with self.lock:
            if resource_id not in self.locks:
                self.locks[resource_id] = process_id
                self.held_locks[process_id].add(resource_id)
                return True
            
            # Resource ist bereits gelockt
            request = LockRequest(
                resource_id=resource_id,
                process_id=process_id,
                timestamp=time.time()
            )
            self.waiting[resource_id].append(request)
            
            # Prüfe auf Deadlocks nach jedem neuen Wait
            if self._check_deadlock():
                self._resolve_deadlock()
                
            return False

    def release_lock(self, resource_id: str, process_id: str) -> None:
        """Gibt einen Lock frei und verarbeitet wartende Requests."""
        with self.lock:
            if resource_id in self.locks and self.locks[resource_id] == process_id:
                self.held_locks[process_id].remove(resource_id)
                del self.locks[resource_id]
                
                # Verarbeite wartende Requests
                if resource_id in self.waiting and self.waiting[resource_id]:
                    next_request = self.waiting[resource_id].pop(0)
                    self.locks[resource_id] = next_request.process_id
                    self.held_locks[next_request.process_id].add(resource_id)

    def _check_deadlock(self) -> bool:
        """
        Erkennt Deadlocks mittels Zykluserkennung im Wait-For-Graph.
        Returns True wenn ein Deadlock gefunden wurde.
        """
        # Baue Wait-For-Graph
        wait_for_graph: Dict[str, Set[str]] = defaultdict(set)
        
        for resource_id, waiters in self.waiting.items():
            if not waiters:
                continue
            current_holder = self.locks[resource_id]
            for waiter in waiters:
                wait_for_graph[waiter.process_id].add(current_holder)
        
        # Suche nach Zyklen mittels DFS
        def has_cycle(node: str, visited: Set[str], path: Set[str]) -> bool:
            if node in path:
                return True
            if node in visited:
                return False
                
            visited.add(node)
            path.add(node)
            
            for neighbor in wait_for_graph[node]:
                if has_cycle(neighbor, visited, path):
                    return True
                    
            path.remove(node)
            return False
        
        visited: Set[str] = set()
        for node in wait_for_graph:
            if node not in visited:
                if has_cycle(node, visited, set()):
                    return True
        return False

    def _resolve_deadlock(self) -> None:
        """
        Löst Deadlocks auf indem der jüngste Request abgebrochen wird.
        """
        youngest_request = None
        youngest_timestamp = float('-inf')
        
        for waiters in self.waiting.values():
            for request in waiters:
                if request.timestamp > youngest_timestamp:
                    youngest_timestamp = request.timestamp
                    youngest_request = request
        
        if youngest_request:
            # Entferne den jüngsten Request aus der Warteliste
            self.waiting[youngest_request.resource_id].remove(youngest_request)

class ResourceManager:
    def __init__(self):
        self.lock_manager = DistributedLockManager()
        self.process_id = str(uuid.uuid4())
        
    def acquire_resource(self, resource_id: str, timeout: float = 5.0) -> bool:
        """
        Versucht eine Ressource zu akquirieren mit Timeout.
        Returns True wenn erfolgreich, False bei Timeout.
        """
        start_time = time.time()
        while time.time() - start_time < timeout:
            if self.lock_manager.request_lock(resource_id, self.process_id):
                return True
            time.sleep(0.1)
        return False
        
    def release_resource(self, resource_id: str) -> None:
        """Gibt eine Ressource frei."""
        self.lock_manager.release_lock(resource_id, self.process_id)

# Beispiel Verwendung
def example_usage():
    # Erstelle zwei Resource Manager Instanzen
    rm1 = ResourceManager()
    rm2 = ResourceManager()
    
    # Ressource IDs
    resource_a = "resource_a"
    resource_b = "resource_b"
    
    # Process 1 akquiriert Resource A
    print("Process 1 versucht Resource A zu akquirieren...")
    success1 = rm1.acquire_resource(resource_a)
    print(f"Process 1 Resource A Akquirierung: {'erfolgreich' if success1 else 'fehlgeschlagen'}")
    
    # Process 2 akquiriert Resource B
    print("Process 2 versucht Resource B zu akquirieren...")
    success2 = rm2.acquire_resource(resource_b)
    print(f"Process 2 Resource B Akquirierung: {'erfolgreich' if success2 else 'fehlgeschlagen'}")
    
    # Versuche einen Deadlock zu erzeugen
    print("Process 1 versucht Resource B zu akquirieren...")
    success3 = rm1.acquire_resource(resource_b)
    print(f"Process 1 Resource B Akquirierung: {'erfolgreich' if success3 else 'fehlgeschlagen'}")
    
    print("Process 2 versucht Resource A zu akquirieren...")
    success4 = rm2.acquire_resource(resource_a)
    print(f"Process 2 Resource A Akquirierung: {'erfolgreich' if success4 else 'fehlgeschlagen'}")
    
    # Ressourcen freigeben
    rm1.release_resource(resource_a)
    rm1.release_resource(resource_b)
    rm2.release_resource(resource_a)
    rm2.release_resource(resource_b)

if __name__ == "__main__":
    example_usage()