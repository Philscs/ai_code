from datetime import datetime, timedelta
from typing import List, Dict, Optional
from enum import Enum
from dataclasses import dataclass
import uuid

class BookStatus(Enum):
    AVAILABLE = "verfügbar"
    BORROWED = "ausgeliehen"
    RESERVED = "reserviert"
    OVERDUE = "überfällig"

@dataclass
class Book:
    isbn: str
    title: str
    author: str
    genre: str
    publication_year: int
    copy_id: str = None

    def __post_init__(self):
        if not self.copy_id:
            self.copy_id = str(uuid.uuid4())

class BookCopy:
    def __init__(self, book: Book):
        self.book = book
        self.status = BookStatus.AVAILABLE
        self.current_borrower = None
        self.due_date = None
        self.reservation = None

class Member:
    def __init__(self, member_id: str, name: str, email: str):
        self.member_id = member_id
        self.name = name
        self.email = email
        self.borrowed_books: List[BookCopy] = []
        self.reservations: List[BookCopy] = []
        self.overdue_notices: List[str] = []

class Library:
    def __init__(self):
        self.books: Dict[str, List[BookCopy]] = {}  # ISBN -> List of copies
        self.members: Dict[str, Member] = {}
        self.loan_period = timedelta(days=30)
        self.max_renewals = 2

    def add_book(self, book: Book, copies: int = 1):
        """Fügt ein neues Buch mit der angegebenen Anzahl von Kopien hinzu."""
        if book.isbn not in self.books:
            self.books[book.isbn] = []
        
        for _ in range(copies):
            new_copy = BookCopy(Book(
                book.isbn, book.title, book.author, 
                book.genre, book.publication_year
            ))
            self.books[book.isbn].append(new_copy)

    def add_member(self, member: Member):
        """Registriert ein neues Bibliotheksmitglied."""
        self.members[member.member_id] = member

    def find_books(self, **kwargs) -> List[Book]:
        """Sucht Bücher nach verschiedenen Kriterien."""
        results = []
        for copies in self.books.values():
            for copy in copies:
                book = copy.book
                match = True
                for key, value in kwargs.items():
                    if hasattr(book, key) and value.lower() not in str(getattr(book, key)).lower():
                        match = False
                        break
                if match and book not in results:
                    results.append(book)
        return results

    def borrow_book(self, isbn: str, member_id: str) -> bool:
        """Leiht ein Buch an ein Mitglied aus."""
        if isbn not in self.books or member_id not in self.members:
            return False

        member = self.members[member_id]
        available_copy = None

        for copy in self.books[isbn]:
            if copy.status == BookStatus.AVAILABLE:
                if copy.reservation is None or copy.reservation == member_id:
                    available_copy = copy
                    break

        if available_copy:
            available_copy.status = BookStatus.BORROWED
            available_copy.current_borrower = member_id
            available_copy.due_date = datetime.now() + self.loan_period
            available_copy.reservation = None
            member.borrowed_books.append(available_copy)
            return True

        return False

    def return_book(self, isbn: str, copy_id: str) -> bool:
        """Nimmt ein ausgeliehenes Buch zurück."""
        if isbn not in self.books:
            return False

        for copy in self.books[isbn]:
            if copy.book.copy_id == copy_id and copy.status == BookStatus.BORROWED:
                member = self.members[copy.current_borrower]
                member.borrowed_books.remove(copy)
                copy.status = BookStatus.AVAILABLE
                copy.current_borrower = None
                copy.due_date = None
                return True

        return False

    def reserve_book(self, isbn: str, member_id: str) -> bool:
        """Reserviert ein Buch für ein Mitglied."""
        if isbn not in self.books or member_id not in self.members:
            return False

        # Prüfe, ob bereits alle Exemplare reserviert sind
        all_copies = self.books[isbn]
        reserved_copies = sum(1 for copy in all_copies if copy.reservation is not None)
        
        if reserved_copies < len(all_copies):
            for copy in all_copies:
                if copy.reservation is None:
                    copy.reservation = member_id
                    self.members[member_id].reservations.append(copy)
                    return True

        return False

    def check_overdue(self):
        """Überprüft überfällige Bücher und erstellt Mahnungen."""
        current_date = datetime.now()
        
        for member in self.members.values():
            for book in member.borrowed_books:
                if book.due_date and current_date > book.due_date:
                    book.status = BookStatus.OVERDUE
                    overdue_days = (current_date - book.due_date).days
                    notice = f"Mahnung: Das Buch '{book.book.title}' ist seit {overdue_days} Tagen überfällig."
                    if notice not in member.overdue_notices:
                        member.overdue_notices.append(notice)

    def extend_loan(self, isbn: str, copy_id: str) -> bool:
        """Verlängert die Ausleihe eines Buches."""
        if isbn not in self.books:
            return False

        for copy in self.books[isbn]:
            if copy.book.copy_id == copy_id and copy.status == BookStatus.BORROWED:
                # Prüfe, ob das Buch reserviert ist
                if copy.reservation is not None:
                    return False
                
                # Prüfe, ob das Buch überfällig ist
                if copy.status == BookStatus.OVERDUE:
                    return False
                
                copy.due_date += self.loan_period
                return True

        return False

# Beispiel zur Verwendung des Systems:
if __name__ == "__main__":
    # Initialisiere die Bibliothek
    library = Library()
    
    # Füge Bücher hinzu
    book1 = Book("978-3-456-78901-2", "Der Prozess", "Franz Kafka", "Roman", 1925)
    book2 = Book("978-3-456-78902-9", "Faust", "Johann Wolfgang von Goethe", "Drama", 1808)
    
    library.add_book(book1, copies=3)
    library.add_book(book2, copies=2)
    
    # Registriere Mitglieder
    member1 = Member("M001", "Max Mustermann", "max@example.com")
    member2 = Member("M002", "Anna Schmidt", "anna@example.com")
    
    library.add_member(member1)
    library.add_member(member2)
    
    # Beispiel-Operationen
    # Buch ausleihen
    library.borrow_book("978-3-456-78901-2", "M001")
    
    # Buch reservieren
    library.reserve_book("978-3-456-78902-9", "M002")
    
    # Suche nach Büchern
    kafka_books = library.find_books(author="Kafka")
    drama_books = library.find_books(genre="Drama")
    
    # Überprüfe überfällige Bücher
    library.check_overdue()