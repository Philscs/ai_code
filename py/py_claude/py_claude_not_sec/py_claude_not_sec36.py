import logging
import uuid
from enum import Enum
from typing import Dict, List, Optional
import pickle
import os
from datetime import datetime

# Konfiguration des Loggings
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class TransactionState(Enum):
    INIT = "INIT"
    PREPARED = "PREPARED"
    COMMITTED = "COMMITTED"
    ABORTED = "ABORTED"

class Transaction:
    def __init__(self, tx_id: str = None):
        self.tx_id = tx_id or str(uuid.uuid4())
        self.state = TransactionState.INIT
        self.participants: List[str] = []
        self.start_time = datetime.now()
        
    def add_participant(self, participant_id: str):
        if participant_id not in self.participants:
            self.participants.append(participant_id)
            
    def to_dict(self):
        return {
            'tx_id': self.tx_id,
            'state': self.state.value,
            'participants': self.participants,
            'start_time': self.start_time.isoformat()
        }

class TransactionManager:
    def __init__(self, manager_id: str, log_dir: str = "tx_logs"):
        self.manager_id = manager_id
        self.active_transactions: Dict[str, Transaction] = {}
        self.log_dir = log_dir
        self.ensure_log_directory()
        self.recover_state()

    def ensure_log_directory(self):
        """Erstellt das Log-Verzeichnis falls es nicht existiert"""
        if not os.path.exists(self.log_dir):
            os.makedirs(self.log_dir)

    def get_log_path(self, tx_id: str) -> str:
        """Gibt den Pfad zur Log-Datei für eine bestimmte Transaktion zurück"""
        return os.path.join(self.log_dir, f"tx_{tx_id}.log")

    def log_transaction(self, transaction: Transaction):
        """Schreibt den Transaktionsstatus in eine Log-Datei"""
        log_path = self.get_log_path(transaction.tx_id)
        with open(log_path, 'wb') as f:
            pickle.dump(transaction.to_dict(), f)

    def recover_state(self):
        """Stellt den Zustand aus den Log-Dateien wieder her"""
        logger.info(f"Starting recovery for manager {self.manager_id}")
        for filename in os.listdir(self.log_dir):
            if filename.startswith("tx_") and filename.endswith(".log"):
                try:
                    with open(os.path.join(self.log_dir, filename), 'rb') as f:
                        tx_data = pickle.load(f)
                        tx = Transaction(tx_data['tx_id'])
                        tx.state = TransactionState(tx_data['state'])
                        tx.participants = tx_data['participants']
                        tx.start_time = datetime.fromisoformat(tx_data['start_time'])
                        self.active_transactions[tx.tx_id] = tx
                        logger.info(f"Recovered transaction {tx.tx_id} in state {tx.state}")
                except Exception as e:
                    logger.error(f"Error recovering transaction from {filename}: {e}")

    def start_transaction(self) -> Transaction:
        """Startet eine neue Transaktion"""
        tx = Transaction()
        self.active_transactions[tx.tx_id] = tx
        self.log_transaction(tx)
        logger.info(f"Started new transaction {tx.tx_id}")
        return tx

    def prepare_transaction(self, tx_id: str) -> bool:
        """Phase 1: Prepare"""
        if tx_id not in self.active_transactions:
            return False
        
        tx = self.active_transactions[tx_id]
        if not tx.participants:
            logger.warning(f"Transaction {tx_id} has no participants")
            return False

        # Hier würde normalerweise die Kommunikation mit den Teilnehmern stattfinden
        # und deren Prepare-Responses gesammelt werden
        
        tx.state = TransactionState.PREPARED
        self.log_transaction(tx)
        logger.info(f"Transaction {tx_id} prepared successfully")
        return True

    def commit_transaction(self, tx_id: str) -> bool:
        """Phase 2: Commit"""
        if tx_id not in self.active_transactions:
            return False
        
        tx = self.active_transactions[tx_id]
        if tx.state != TransactionState.PREPARED:
            logger.warning(f"Cannot commit transaction {tx_id} in state {tx.state}")
            return False

        # Hier würde normalerweise die Commit-Nachricht an alle Teilnehmer gesendet
        
        tx.state = TransactionState.COMMITTED
        self.log_transaction(tx)
        logger.info(f"Transaction {tx_id} committed successfully")
        return True

    def abort_transaction(self, tx_id: str) -> bool:
        """Bricht eine Transaktion ab"""
        if tx_id not in self.active_transactions:
            return False
        
        tx = self.active_transactions[tx_id]
        
        # Hier würde normalerweise die Abort-Nachricht an alle Teilnehmer gesendet
        
        tx.state = TransactionState.ABORTED
        self.log_transaction(tx)
        logger.info(f"Transaction {tx_id} aborted")
        return True

    def cleanup_transaction(self, tx_id: str):
        """Entfernt eine abgeschlossene Transaktion aus dem System"""
        if tx_id in self.active_transactions:
            tx = self.active_transactions[tx_id]
            if tx.state in [TransactionState.COMMITTED, TransactionState.ABORTED]:
                del self.active_transactions[tx_id]
                log_path = self.get_log_path(tx_id)
                if os.path.exists(log_path):
                    os.remove(log_path)
                logger.info(f"Cleaned up transaction {tx_id}")

class ResourceManager:
    def __init__(self, resource_id: str):
        self.resource_id = resource_id
        self.data = {}
        self.temp_data = {}
        
    def prepare(self, tx_id: str) -> bool:
        """Bereitet die Änderungen für das Commit vor"""
        if tx_id not in self.temp_data:
            return False
        # Hier würden Ressourcen gesperrt und Änderungen vorbereitet
        return True
        
    def commit(self, tx_id: str) -> bool:
        """Führt die Änderungen endgültig durch"""
        if tx_id not in self.temp_data:
            return False
        self.data.update(self.temp_data[tx_id])
        del self.temp_data[tx_id]
        return True
        
    def abort(self, tx_id: str) -> bool:
        """Verwirft die vorbereiteten Änderungen"""
        if tx_id in self.temp_data:
            del self.temp_data[tx_id]
        return True

# Beispiel für die Verwendung
def example_usage():
    # Initialisierung der Manager
    tx_manager = TransactionManager("TM1")
    resource1 = ResourceManager("RS1")
    resource2 = ResourceManager("RS2")
    
    # Starte neue Transaktion
    tx = tx_manager.start_transaction()
    
    try:
        # Füge Teilnehmer hinzu
        tx.add_participant(resource1.resource_id)
        tx.add_participant(resource2.resource_id)
        
        # Phase 1: Prepare
        if tx_manager.prepare_transaction(tx.tx_id):
            # Phase 2: Commit
            if tx_manager.commit_transaction(tx.tx_id):
                logger.info("Transaction completed successfully")
            else:
                logger.error("Commit failed")
                tx_manager.abort_transaction(tx.tx_id)
        else:
            logger.error("Prepare failed")
            tx_manager.abort_transaction(tx.tx_id)
            
    except Exception as e:
        logger.error(f"Error during transaction: {e}")
        tx_manager.abort_transaction(tx.tx_id)
    
    # Cleanup
    tx_manager.cleanup_transaction(tx.tx_id)

if __name__ == "__main__":
    example_usage()