import random

class Rule:
    def apply(self, record):
        """
        Anwendet die Regel auf eine Datenzeile.

        :param record: Die Datenzeile als dictionary.
        :return: Die bearbeitete Datenzeile als dictionary.
        """
        pass

class MaxValueRule(Rule):
    def __init__(self, column, max_value):
        """
        Erstellt eine Regel, die sicherstellt, dass ein bestimmter Wert nicht größer als ein 
Maximum ist.

        :param column: Der Name der Spalte.
        :param max_value: Das Maximum-Wert.
        """
        self.column = column
        self.max_value = max_value

    def apply(self, record):
        if record.get(self.column) > self.max_value:
            raise ValueError(f"Maximalwert {self.max_value} überschritten")
        return record

class MinValueRule(Rule):
    def __init__(self, column, min_value):
        """
        Erstellt eine Regel, die sicherstellt, dass ein bestimmter Wert nicht kleiner als ein 
Minimum ist.

        :param column: Der Name der Spalte.
        :param min_value: Das Minimum-Wert.
        """
        self.column = column
        self.min_value = min_value

    def apply(self, record):
        if record.get(self.column) < self.min_value:
            raise ValueError(f"Minimalwert {self.min_value} überschritten")
        return record

class MockDataGenerator:
    def __init__(self, db_schema):
        """
        Erstellt ein MockData-System basierend auf dem Datenbankschema.

        :param db_schema: Das Datenbankschema als dictionary.
        """
        self.db_schema = db_schema
        self.data = {}

    def add_table(self, table_name, num_records=10):
        """
        Fügt eine Tabelle zum MockData-System hinzu.

        :param table_name: Der Name der Tabelle.
        :param num_records: Die Anzahl der Datenzeilen pro Tabelle. Standardwert ist 10.
        """
        self.data[table_name] = []
        for _ in range(num_records):
            record = {}
            for column, data_type in self.db_schema.get(table_name, {}).items():
                if data_type == 'int':
                    record[column] = random.randint(0, 100)
                elif data_type == 'str':
                    record[column] = ''.join(random.choice('abcdefghijklmnopqrstuvwxyz') for _ in 
range(10))
                else:
                    raise ValueError(f"Ungültiger Datentyp {data_type}")
            self.data[table_name].append(record)

    def add_record(self, table_name, record):
        """
        Fügt eine Datenzeile zu einer Tabelle hinzu.

        :param table_name: Der Name der Tabelle.
        :param record: Die Datenzeile als dictionary.
        """
        if table_name in self.data:
            self.data[table_name].append(record)

    def get_data(self):
        """
        Gibt das gesamte MockData-System zurück.

        :return: Das gesamte MockData-System als dictionary.
        """
        return self.data

# Beispielanwendung
db_schema = {
    'users': {'id': 'int', 'name': 'str', 'email': 'str'},
    'orders': {'id': 'int', 'user_id': 'int', 'order_date': 'date'}
}

generator = MockDataGenerator(db_schema)
max_value_rule = MaxValueRule('age', 100)
min_value_rule = MinValueRule('weight', 50)

def apply_rules(record):
    try:
        record = max_value_rule.apply(record)
        record = min_value_rule.apply(record)
    except ValueError as e:
        print(f"Fehler: {e}")
        return None
    return record

generator.add_table('users')
for _ in range(10):
    user_record = {'id': random.randint(0, 100), 'name': 
''.join(random.choice('abcdefghijklmnopqrstuvwxyz') for _ in range(10)), 'email': 
f'user{random.randint(0, 100)}@example.com'}
    while True:
        user_record = apply_rules(user_record)
        if user_record is not None:
            break

print(generator.get_data())
