from typing import Any, Callable, Dict, List, Optional, Union

class ValidationError(Exception):
    """Custom exception for validation errors."""
    def __init__(self, message: str, field: Optional[str] = None):
        self.message = message
        self.field = field
        super().__init__(message)

class DataValidator:
    """Generic Data Validator for validating complex rules."""

    def __init__(self, locale: str = "en"):
        self.locale = locale
        self.error_messages = {
            "en": {
                "required": "{field} is required.",
                "type": "{field} must be of type {type}.",
                "custom": "{field} failed custom validation: {error}.",
            },
            "de": {
                "required": "{field} ist erforderlich.",
                "type": "{field} muss vom Typ {type} sein.",
                "custom": "{field} hat die benutzerdefinierte Validierung nicht bestanden: {error}.",
            },
        }

    def validate(self, data: Dict[str, Any], schema: Dict[str, Any]) -> None:
        """Validate data against a schema."""
        for field, rules in schema.items():
            value = data.get(field)
            if "required" in rules and rules["required"] and value is None:
                self._raise_error("required", field)
            if "type" in rules and value is not None and not isinstance(value, rules["type"]):
                self._raise_error("type", field, rules["type"].__name__)
            if "custom" in rules and callable(rules["custom"]):
                try:
                    rules["custom"](value)
                except ValidationError as e:
                    self._raise_error("custom", field, str(e))

    def _raise_error(self, error_key: str, field: str, *args: Any) -> None:
        """Raise a localized validation error."""
        template = self.error_messages[self.locale][error_key]
        message = template.format(field=field, *args)
        raise ValidationError(message, field=field)

# Beispielbenutzung:
def custom_rule(value: Any) -> None:
    if value is not None and value <= 0:
        raise ValidationError("Value must be greater than 0.")

schema = {
    "name": {"required": True, "type": str},
    "age": {"required": True, "type": int, "custom": custom_rule},
    "email": {"type": str},
}

data = {
    "name": "Anna",
    "age": -5,
}

validator = DataValidator(locale="de")
try:
    validator.validate(data, schema)
except ValidationError as e:
    print(f"Validation Error: {e.message} (Field: {e.field})")
