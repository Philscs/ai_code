import abc
from typing import Dict, Any

class Constraint(abc.ABC):
    @abc.abstractmethod
    def validate(self, value: Any) -> bool:
        pass

class RequiredConstraint(Constraint):
    def __init__(self, field_name: str):
        self.field_name = field_name

    def validate(self, value: Any) -> bool:
        return value is not None and len(value) > 0

class IntegerConstraint(Constraint):
    def __init__(self, field_name: str, min_value: int, max_value: int):
        self.field_name = field_name
        self.min_value = min_value
        self.max_value = max_value

    def validate(self, value: Any) -> bool:
        return isinstance(value, int) and self.min_value <= value <= self.max_value

class CustomConstraint(Constraint):
    def __init__(self, field_name: str, custom_constraint: callable):
        self.field_name = field_name
        self.custom_constraint = custom_constraint

    def validate(self, value: Any) -> bool:
        return self.custom_constraint(value)

class DataValidator:
    def __init__(self, constraints: Dict[str, Constraint]):
        self.constraints = constraints

    def validate(self, data: Dict[str, Any]) -> None:
        for field_name, constraint in self.constraints.items():
            value = data.get(field_name)
            if not constraint.validate(value):
                raise ValueError(f"Invalid value for '{field_name}': {value}")

def main():
    # Erstelle ein Data-Objekt
    data = {
        "name": "",
        "age": 25,
        "email": ""
    }

    # Erstelle Constraints
    constraints = [
        RequiredConstraint("name"),
        IntegerConstraint("age", min_value=18, max_value=100),
        CustomConstraint("email", lambda x: x.endswith("@example.com"))
    ]

    # Erstelle Data-Validator
    validator = DataValidator(constraints)

    try:
        # Valide Daten
        validator.validate(data)
        print("Daten sind gültig")
    except ValueError as e:
        print(e)

if __name__ == "__main__":
    main()
