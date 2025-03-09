class Book:
    def __init__(self, title, author, isbn, genre, copies):
        self.title = title
        self.author = author
        self.isbn = isbn
        self.genre = genre
        self.copies = copies

class Borrower:
    def __init__(self, name, email):
        self.name = name
        self.email = email

class LoanTransaction:
    def __init__(self, book, borrower, due_date):
        self.book = book
        self.borrower = borrower
        self.due_date = due_date
class Library:
    def __init__(self):
        self.books = []
        self.borrowers = []
        self.transactions = []

    def add_book(self, book):
        self.books.append(book)

    def add_borrower(self, borrower):
        self.borrowers.append(borrower)

    def add_transaction(self, transaction):
        self.transactions.append(transaction)

    # Add other methods for managing the library, such as search, reservations, reminders, etc.

# Example usage:
library = Library()

book1 = Book("Book 1", "Author 1", "ISBN 1", "Genre 1", 5)
book2 = Book("Book 2", "Author 2", "ISBN 2", "Genre 2", 3)

borrower1 = Borrower("Borrower 1", "borrower1@example.com")
borrower2 = Borrower("Borrower 2", "borrower2@example.com")

transaction1 = LoanTransaction(book1, borrower1, "2022-12-31")
transaction2 = LoanTransaction(book2, borrower2, "2022-12-31")

library.add_book(book1)
library.add_book(book2)
library.add_borrower(borrower1)
library.add_borrower(borrower2)
library.add_transaction(transaction1)
library.add_transaction(transaction2)
