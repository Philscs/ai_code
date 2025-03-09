from datetime import datetime, timedelta

class Book:
    def __init__(self, title, author, isbn, genre, copies):
        self.title = title
        self.author = author
        self.isbn = isbn
        self.genre = genre
        self.copies = copies

    def __str__(self):
        return f"{self.title} by {self.author} (ISBN: {self.isbn}, Genre: {self.genre}, Copies: {self.copies})"


class User:
    def __init__(self, name, user_id):
        self.name = name
        self.user_id = user_id
        self.borrowed_books = []
        self.reservations = []

    def __str__(self):
        return f"{self.name} (ID: {self.user_id})"


class Loan:
    def __init__(self, book, user, due_date):
        self.book = book
        self.user = user
        self.due_date = due_date
        self.returned = False

    def __str__(self):
        status = "Returned" if self.returned else f"Due: {self.due_date}"
        return f"{self.book.title} borrowed by {self.user.name} ({status})"


class Library:
    def __init__(self):
        self.books = []
        self.users = []
        self.loans = []

    def add_book(self, book):
        self.books.append(book)

    def add_user(self, user):
        self.users.append(user)

    def search_books(self, **criteria):
        results = self.books
        if "title" in criteria:
            results = [book for book in results if criteria["title"].lower() in book.title.lower()]
        if "author" in criteria:
            results = [book for book in results if criteria["author"].lower() in book.author.lower()]
        if "isbn" in criteria:
            results = [book for book in results if book.isbn == criteria["isbn"]]
        if "genre" in criteria:
            results = [book for book in results if criteria["genre"].lower() in book.genre.lower()]
        return results

    def borrow_book(self, isbn, user_id):
        book = next((b for b in self.books if b.isbn == isbn and b.copies > 0), None)
        user = next((u for u in self.users if u.user_id == user_id), None)

        if not book:
            print("Book not available.")
            return

        if not user:
            print("User not found.")
            return

        book.copies -= 1
        due_date = datetime.now() + timedelta(days=14)
        loan = Loan(book, user, due_date)
        self.loans.append(loan)
        user.borrowed_books.append(book)
        print(f"Book borrowed: {loan}")

    def return_book(self, isbn, user_id):
        loan = next((l for l in self.loans if l.book.isbn == isbn and l.user.user_id == user_id and not l.returned), None)

        if not loan:
            print("Loan not found.")
            return

        loan.returned = True
        loan.book.copies += 1
        loan.user.borrowed_books.remove(loan.book)
        print(f"Book returned: {loan.book.title}")

    def reserve_book(self, isbn, user_id):
        book = next((b for b in self.books if b.isbn == isbn), None)
        user = next((u for u in self.users if u.user_id == user_id), None)

        if not book:
            print("Book not found.")
            return

        if not user:
            print("User not found.")
            return

        user.reservations.append(book)
        print(f"Book reserved: {book.title} by {user.name}")

    def check_overdue_loans(self):
        overdue_loans = [l for l in self.loans if not l.returned and l.due_date < datetime.now()]
        for loan in overdue_loans:
            print(f"Overdue: {loan}")


# Example Usage
library = Library()

# Adding books
library.add_book(Book("1984", "George Orwell", "1234567890", "Dystopian", 5))
library.add_book(Book("To Kill a Mockingbird", "Harper Lee", "0987654321", "Fiction", 3))

# Adding users
library.add_user(User("Alice", 1))
library.add_user(User("Bob", 2))

# Borrowing books
library.borrow_book("1234567890", 1)
library.borrow_book("1234567890", 2)

# Returning books
library.return_book("1234567890", 1)

# Reserving books
library.reserve_book("0987654321", 2)

# Searching books
results = library.search_books(title="1984")
print("Search results:")
for book in results:
    print(book)

# Checking overdue loans
library.check_overdue_loans()
