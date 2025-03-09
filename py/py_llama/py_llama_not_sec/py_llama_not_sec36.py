import uuid
from abc import ABC, abstractmethod

class Transaction(ABC):
    @abstractmethod
    def prepare(self):
        pass

class TransactionManager:
    def __init__(self):
        self.txns = {}

    def begin_txn(self):
        txn_id = str(uuid.uuid4())
        print(f"TXN {txn_id} begonnen")
        self.txns[txn_id] = Transaction()
        return txn_id

    def commit_txn(self, txn_id):
        if txn_id in self.txns:
            print(f"TXN {txn_id} kommittiert")
            del self.txns[txn_id]
        else:
            print("TXN nicht gefunden")

    def rollback_txn(self, txn_id):
        if txn_id in self.txns:
            print(f"TXN {txn_id} zurückgezogen")
            del self.txns[txn_id]
        else:
            print("TXN nicht gefunden")

class MyTransaction(Transaction):
    def prepare(self):
        # Simulating some work being done during preparation
        print("Preparing TXN...")
        # This will be the last step before we can commit
        pass

class TransactionResource:
    def __init__(self, name):
        self.name = name

    def execute(self, txn):
        if not txn.prepare():
            return False
        else:
            print(f"Executed {self.name}")
            return True


# Create resources
resource1 = TransactionResource("resource1")
resource2 = TransactionResource("resource2")

# Create transaction manager
txn_manager = TransactionManager()

# Begin new TXN
txn_id = txn_manager.begin_txn()

# Prepare TXN
my_txn = MyTransaction()
if not my_txn.prepare():
    print(f"TXN {txn_id} Fehlschlag")
else:
    # Execute resources and commit TXN
    if resource1.execute(my_txn) and resource2.execute(my_txn):
        txn_manager.commit_txn(txn_id)
    else:
        txn_manager.rollback_txn(txn_id)