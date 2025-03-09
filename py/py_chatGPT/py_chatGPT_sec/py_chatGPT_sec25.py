from typing import Any, Type, get_type_hints

class TypeCheckError(Exception):
    """Exception raised when a type check fails."""
    pass

class CustomTypeChecker:
    def __init__(self, constraints: dict = None):
        """Initialize the type checker with optional constraints.

        Args:
            constraints (dict): A dictionary mapping type names to validation functions.
        """
        self.constraints = constraints or {}

    def validate(self, instance: Any, expected_type: Type):
        """Validate the instance against the expected type.

        Args:
            instance (Any): The value to check.
            expected_type (Type): The expected type.

        Raises:
            TypeCheckError: If the type validation fails.
        """
        if not isinstance(instance, expected_type):
            raise TypeCheckError(f"Expected type {expected_type.__name__}, but got {type(instance).__name__}.")

        # Check for custom constraints
        type_name = expected_type.__name__
        if type_name in self.constraints:
            constraint_func = self.constraints[type_name]
            if not constraint_func(instance):
                raise TypeCheckError(f"Value {instance} does not satisfy the constraint for type {type_name}.")

    def check_function(self, func):
        """Decorator to enforce type checks on function arguments and return value.

        Args:
            func (callable): The function to wrap.

        Returns:
            callable: The wrapped function.
        """
        def wrapper(*args, **kwargs):
            hints = get_type_hints(func)

            # Validate arguments
            arg_names = func.__code__.co_varnames
            for i, arg in enumerate(args):
                arg_name = arg_names[i]
                if arg_name in hints:
                    self.validate(arg, hints[arg_name])

            for kwarg_name, kwarg_value in kwargs.items():
                if kwarg_name in hints:
                    self.validate(kwarg_value, hints[kwarg_name])

            # Call the original function
            result = func(*args, **kwargs)

            # Validate return value
            if 'return' in hints:
                self.validate(result, hints['return'])

            return result

        return wrapper

# Example usage
if __name__ == "__main__":
    # Custom constraints
    def positive_integer(value):
        return isinstance(value, int) and value > 0

    checker = CustomTypeChecker(constraints={
        "PositiveInt": positive_integer
    })

    @checker.check_function
    def add_numbers(a: int, b: "PositiveInt") -> int:
        return a + b

    try:
        print(add_numbers(5, 10))  # Valid case
        print(add_numbers(5, -10))  # Invalid case
    except TypeCheckError as e:
        print(f"Type check failed: {e}")