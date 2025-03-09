class ValidationError(Exception):
    pass

class DataValidator:
    def __init__(self, constraints=None):
        self.constraints = constraints or []

    def add_constraint(self, constraint):
        self.constraints.append(constraint)

    def validate(self, data):
        errors = []
        for constraint in self.constraints:
            try:
                constraint.validate(data)
            except ValidationError as e:
                errors.append(str(e))
        if errors:
            raise ValidationError("\n".join(errors))

class Constraint:
    def validate(self, data):
        raise NotImplementedError()

class LengthConstraint(Constraint):
    def __init__(self, min_length, max_length):
        self.min_length = min_length
        self.max_length = max_length

    def validate(self, data):
        if len(data) < self.min_length or len(data) > self.max_length:
            raise ValidationError(f"Length must be between {self.min_length} and {self.max_length}")

# Example usage
validator = DataValidator()
validator.add_constraint(LengthConstraint(5, 10))

try:
    validator.validate("hello")
    print("Validation successful")
except ValidationError as e:
    print(f"Validation failed: {str(e)}")
