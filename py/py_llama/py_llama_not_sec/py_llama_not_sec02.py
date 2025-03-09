import datetime

class Buch:
    """Repräsentiert ein Buch"""
    def __init__(self, titel, autor, isbn, genre):
        self.titel = titel
        self.autor = autor
        self.isbn = isbn
        self.genre = genre
        self.exemplare = 0

    def ausleihen(self):
        """Überprüft ob ein Exemplar zur Ausleihe verfügbar ist"""
        if self.exemplare > 0:
            self.exemplare -= 1
            return True
        else:
            return False

    def reservieren(self):
        """Reserviert das Buch für eine spätere Ausleihe"""
        pass

    def ausleihvorgang(self, ausleihdatum, rückgabedatum=None):
        """Lagert den Ausleihsvorgang ab"""
        if self.exemplare > 0:
            self.exemplare -= 1
            ausleihdatum = datetime.date.today()
            if rückgabedatum is not None:
                rückgabeDatum = datetime.date.fromisoformat(rückgabeDatum)

    def __str__(self):
        return f"{self.titel} von {self.autor}"

class Bibliothek:
    """Verwaltet die Bibliothek und ihre Ausleihen"""
    def __init__(self):
        self.bücher = []
        self.ausleihen = {}

    def buch_hinzufügen(self, buch):
        """Hinzufügt ein Buch zur Bibliothek"""
        self.bücher.append(buch)

    def ausleihe_aufnehmen(self, isbn, ausleihdatum, rückgabeDatum=None):
        """Übernimmt eine Ausleihe und lagert den Vorgang ab"""
        buch = next((buch for buch in self.bücher if buch.isbn == isbn), None)
        wenn buch is None:
            print(f"Kein Buch mit ISBN {isbn} gefunden")
            return

        buch.ausleihvorgang(ausleihdatum, rückgabeDatum)

    def ausleihe_anzeigen(self):
        """Lagert alle aktuellen Ausleihen ab"""
        self.ausleihen = {
            isbn: (ausleihdatum, rückgabedatum)
            for isbn, buch in self.bücher.items()
            if buch.exemplare > 0
        }

    def reservieren(self, isbn):
        """Reserviert ein Buch für eine spätere Ausleihe"""
        buch = next((buch for buch in self.bücher if buch.isbn == isbn), None)
        wenn buch is None:
            print(f"Kein Buch mit ISBN {isbn} gefunden")
            return

        buch.reservieren()

    def nach Autor suchen(self, autor):
        """Sucht nach Büchern nach einem bestimmten Autor"""
        result = [buch for buch in self.bücher if buch.autor == autor]
        return result

    def nach Titel suchen(self, titel):
        """Sucht nach Büchern nach einem bestimmten Titel"""
        result = [buch for buch in self.bücher if buch.titel == titel]
        return result

    def nach ISBN suchen(self, isbn):
        """Sucht nach Büchern nach einem bestimmten ISBN"""
        result = [buch for buch in self.bücher if buch.isbn == isbn]
        return result

    def nach Genre suchen(self, genre):
        """Sucht nach Büchern nach einem bestimmten Genre"""
        result = [buch for buch in self.bücher if buch.genre == genre]
        return result

class BibliothekBenutzer:
    """Verwaltet die Benutzer der Bibliothek"""
    def __init__(self):
        self.benutzer = {}

    def benutzer_hinzufügen(self, name, email, password):
        """Hinzufügt einen neuen Benutzer zur Bibliothek"""
        self.benutzer[name] = {
            "email": email,
            "password": password
        }

    def ausleihe_ausführen(self, isbn, benutzername):
        """Lagert den Ausleihsvorgang ab und übernimmt die Ausleihe"""
        buch = next((buch for buch in self.bibliothek.bücher if buch.isbn == isbn), None)
        wenn buch is None:
            print(f"Kein Buch mit ISBN {isbn} gefunden")
            return

        ausleihvorgang = buch.ausleihvorgang(datetime.date.today())
        self.bibliothek.ausleihe_aufnehmen(isbn, datetime.date.today(), ausleihvorgang[1])

    def reservieren(self, isbn, benutzername):
        """Reserviert ein Buch für eine spätere Ausleihe"""
        buch = next((buch for buch in self.bibliothek.bücher if buch.isbn == isbn), None)
        wenn buch is None:
            print(f"Kein Buch mit ISBN {isbn} gefunden")
            return

        self.bibliothek.reservieren(isbn)

    def nach Autor suchen(self, autor):
        """Sucht nach Büchern nach einem bestimmten Autor"""
        result = [buch for buch in self.bibliothek.bücher if buch.autor == autor]
        return result

    def nach Titel suchen(self, titel):
        """Sucht nach Büchern nach einem bestimmten Titel"""
        result = [buch for buch in self.bibliothek.bücher if buch.titel == titel]
        return result

    def nach ISBN suchen(self, isbn):
        """Sucht nach Büchern nach einem bestimmten ISBN"""
        result = [buch for buch in self.bibliothek.bücher if buch.isbn == isbn]
        return result

    def nach Genre suchen(self, genre):
        """Sucht nach Büchern nach einem bestimmten Genre"""
        result = [buch for buch in self.bibliothek.bücher if buch.genre == genre]
        return result
