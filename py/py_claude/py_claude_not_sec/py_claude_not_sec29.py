from typing import Dict, List, Any, Optional
import random
import string
from datetime import datetime, timedelta
import json
from dataclasses import dataclass
from abc import ABC, abstractmethod

@dataclass
class FieldRule:
    field_type: str
    min_value: Optional[Any] = None
    max_value: Optional[Any] = None
    choices: Optional[List[Any]] = None
    pattern: Optional[str] = None
    unique: bool = False
    nullable: bool = False
    null_probability: float = 0.1

class DataGenerator(ABC):
    @abstractmethod
    def generate(self) -> Any:
        pass

class IntegerGenerator(DataGenerator):
    def __init__(self, rule: FieldRule):
        self.rule = rule
        self.min_value = rule.min_value if rule.min_value is not None else 0
        self.max_value = rule.max_value if rule.max_value is not None else 1000000
        self.used_values = set() if rule.unique else None

    def generate(self) -> Optional[int]:
        if self.rule.nullable and random.random() < self.rule.null_probability:
            return None

        if self.rule.choices:
            return random.choice(self.rule.choices)

        if self.rule.unique:
            available_values = set(range(self.min_value, self.max_value + 1)) - self.used_values
            if not available_values:
                raise ValueError("No more unique values available")
            value = random.choice(list(available_values))
            self.used_values.add(value)
            return value

        return random.randint(self.min_value, self.max_value)

class StringGenerator(DataGenerator):
    def __init__(self, rule: FieldRule):
        self.rule = rule
        self.min_length = rule.min_value if rule.min_value is not None else 5
        self.max_length = rule.max_value if rule.max_value is not None else 50
        self.used_values = set() if rule.unique else None

    def generate(self) -> Optional[str]:
        if self.rule.nullable and random.random() < self.rule.null_probability:
            return None

        if self.rule.choices:
            return random.choice(self.rule.choices)

        if self.rule.pattern:
            # Hier könnte eine komplexere Pattern-basierte Generierung implementiert werden
            return self._generate_from_pattern()

        length = random.randint(self.min_length, self.max_length)
        while True:
            value = ''.join(random.choices(string.ascii_letters + string.digits, k=length))
            if not self.rule.unique or value not in self.used_values:
                if self.rule.unique:
                    self.used_values.add(value)
                return value

    def _generate_from_pattern(self) -> str:
        # Vereinfachte Pattern-Implementierung
        # Könnte erweitert werden für komplexere Regex-Patterns
        return f"pattern_{random.randint(1000, 9999)}"

class DateTimeGenerator(DataGenerator):
    def __init__(self, rule: FieldRule):
        self.rule = rule
        self.min_date = rule.min_value if rule.min_value else datetime(2000, 1, 1)
        self.max_date = rule.max_value if rule.max_value else datetime.now()

    def generate(self) -> Optional[datetime]:
        if self.rule.nullable and random.random() < self.rule.null_probability:
            return None

        if self.rule.choices:
            return random.choice(self.rule.choices)

        time_between_dates = self.max_date - self.min_date
        days_between_dates = time_between_dates.days
        random_days = random.randrange(days_between_dates)
        return self.min_date + timedelta(days=random_days)

class MockDataGenerator:
    def __init__(self, schema: Dict[str, Dict[str, Any]]):
        """
        Initialize the mock data generator with a schema.
        
        Schema format:
        {
            "table_name": {
                "fields": {
                    "field_name": {
                        "type": "integer|string|datetime",
                        "min_value": Optional[Any],
                        "max_value": Optional[Any],
                        "choices": Optional[List[Any]],
                        "pattern": Optional[str],
                        "unique": bool,
                        "nullable": bool,
                        "null_probability": float
                    }
                }
            }
        }
        """
        self.schema = schema
        self.generators = self._create_generators()

    def _create_generators(self) -> Dict[str, Dict[str, DataGenerator]]:
        generators = {}
        for table_name, table_schema in self.schema.items():
            generators[table_name] = {}
            for field_name, field_schema in table_schema["fields"].items():
                rule = FieldRule(**field_schema)
                if rule.field_type == "integer":
                    generators[table_name][field_name] = IntegerGenerator(rule)
                elif rule.field_type == "string":
                    generators[table_name][field_name] = StringGenerator(rule)
                elif rule.field_type == "datetime":
                    generators[table_name][field_name] = DateTimeGenerator(rule)
                else:
                    raise ValueError(f"Unsupported field type: {rule.field_type}")
        return generators

    def generate_table_data(self, table_name: str, num_records: int) -> List[Dict[str, Any]]:
        """Generate mock data for a specific table."""
        if table_name not in self.schema:
            raise ValueError(f"Table {table_name} not found in schema")

        records = []
        for _ in range(num_records):
            record = {}
            for field_name, generator in self.generators[table_name].items():
                record[field_name] = generator.generate()
            records.append(record)
        return records

    def generate_all_data(self, records_per_table: Dict[str, int]) -> Dict[str, List[Dict[str, Any]]]:
        """Generate mock data for all tables in the schema."""
        data = {}
        for table_name, num_records in records_per_table.items():
            data[table_name] = self.generate_table_data(table_name, num_records)
        return data

# Beispiel für die Verwendung:
if __name__ == "__main__":
    # Beispiel-Schema
    schema = {
        "users": {
            "fields": {
                "id": {
                    "field_type": "integer",
                    "min_value": 1,
                    "unique": True
                },
                "username": {
                    "field_type": "string",
                    "min_value": 5,
                    "max_value": 20,
                    "unique": True
                },
                "email": {
                    "field_type": "string",
                    "pattern": "email",
                    "unique": True
                },
                "created_at": {
                    "field_type": "datetime",
                    "min_value": datetime(2020, 1, 1),
                    "nullable": False
                }
            }
        }
    }

    # Generator erstellen
    generator = MockDataGenerator(schema)

    # Daten generieren
    mock_data = generator.generate_table_data("users", 5)
    
    # Ausgabe der generierten Daten
    print(json.dumps(mock_data, default=str, indent=2))