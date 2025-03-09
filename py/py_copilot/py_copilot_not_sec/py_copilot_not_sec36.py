import uuid
from abc import ABC, abstractmethod

class Transaction(ABC):
    @abstractmethod
    def prepare(self):
        pass

class TransactionManager:
    def __init__(self):
        self.transactions = {}

    def begin_transaction(self):
        transaction_id = str(uuid.uuid4())
        print(f"Transaction {transaction_id} started")
        self.transactions[transaction_id] = Transaction()
        return transaction_id

    def commit_transaction(self, transaction_id):
        if transaction_id in self.transactions:
            print(f"Transaction {transaction_id} committed")
            del self.transactions[transaction_id]
        else:
            print("Transaction not found")

    def rollback_transaction(self, transaction_id):
        if transaction_id in self.transactions:
            print(f"Transaction {transaction_id} rolled back")
            del self.transactions[transaction_id]
        else:
            print("Transaction not found")

class MyTransaction(Transaction):
    def prepare(self):
        # Simulating some work being done during preparation
        print("Preparing transaction...")
        # This will be the last step before we can commit
        pass

class TransactionResource:
    def __init__(self, name):
        self.name = name

    def execute(self, transaction):
        if not transaction.prepare():
            return False
        else:
            print(f"Executed {self.name}")
            return True


# Create resources
resource1 = TransactionResource("resource1")
resource2 = TransactionResource("resource2")

# Create transaction manager
transaction_manager = TransactionManager()

# Begin new transaction
transaction_id = transaction_manager.begin_transaction()

# Prepare transaction
my_transaction = MyTransaction()
if not my_transaction.prepare():
    print(f"Transaction {transaction_id} failed")
else:
    # Execute resources and commit transaction
    if resource1.execute(my_transaction) and resource2.execute(my_transaction):
        transaction_manager.commit_transaction(transaction_id)
    else:
        transaction_manager.rollback_transaction(transaction_id)
