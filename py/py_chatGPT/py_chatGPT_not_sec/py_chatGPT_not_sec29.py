import random
from faker import Faker
from typing import Dict, List, Any, Callable

class MockDataGenerator:
    def __init__(self):
        self.fake = Faker()

    def generate_mock_data(self, schema: Dict[str, str], rules: Dict[str, Callable[[], Any]] = None, num_records: int = 10) -> List[Dict[str, Any]]:
        """
        Generiert Mockdaten basierend auf einem Datenbankschema und optionalen benutzerdefinierten Regeln.

        :param schema: Ein Dictionary, das Feldnamen und deren Typen enthält.
        :param rules: Ein Dictionary mit benutzerdefinierten Regeln für bestimmte Felder.
        :param num_records: Die Anzahl der zu generierenden Datensätze.
        :return: Eine Liste von Dictionaries mit generierten Daten.
        """
        if rules is None:
            rules = {}

        mock_data = []
        for _ in range(num_records):
            record = {}
            for field, field_type in schema.items():
                if field in rules:
                    # Benutzerdefinierte Regel anwenden
                    record[field] = rules[field]()
                else:
                    # Automatisch generieren basierend auf Typ
                    record[field] = self._generate_field_value(field_type)
            mock_data.append(record)

        return mock_data

    def _generate_field_value(self, field_type: str) -> Any:
        """
        Generiert einen Feldwert basierend auf dem Typ.

        :param field_type: Der Typ des Feldes (z. B. 'string', 'integer', 'email').
        :return: Ein generierter Wert basierend auf dem Typ.
        """
        if field_type == 'string':
            return self.fake.word()
        elif field_type == 'integer':
            return random.randint(1, 100)
        elif field_type == 'email':
            return self.fake.email()
        elif field_type == 'name':
            return self.fake.name()
        elif field_type == 'address':
            return self.fake.address()
        elif field_type == 'date':
            return self.fake.date()
        elif field_type == 'boolean':
            return random.choice([True, False])
        else:
            return None  # Standardwert, falls der Typ unbekannt ist

# Beispielnutzung
if __name__ == "__main__":
    schema = {
        "id": "integer",
        "name": "name",
        "email": "email",
        "signup_date": "date",
        "is_active": "boolean",
    }

    # Benutzerdefinierte Regeln
    rules = {
        "id": lambda: random.randint(1000, 9999),
        "is_active": lambda: random.choice([True, False]),
    }

    generator = MockDataGenerator()
    mock_data = generator.generate_mock_data(schema, rules, num_records=5)

    for record in mock_data:
        print(record)
