from typing import Any, Callable, Dict, Type, TypeVar, get_type_hints
from functools import wraps
import inspect

T = TypeVar('T')

class TypeValidationError(Exception):
    """Custom exception for type validation errors."""
    pass

class TypeConstraintError(Exception):
    """Custom exception for constraint validation errors."""
    pass

class TypeChecker:
    def __init__(self):
        self._constraints: Dict[Type, Callable[[Any], bool]] = {}
        
    def register_constraint(self, type_: Type[T], constraint: Callable[[T], bool]) -> None:
        """
        Register a custom constraint for a specific type.
        
        Args:
            type_: The type to constrain
            constraint: A function that takes a value and returns bool
        """
        self._constraints[type_] = constraint

    def validate_type(self, value: Any, expected_type: Type) -> None:
        """
        Validate that a value matches the expected type and its constraints.
        
        Args:
            value: The value to validate
            expected_type: The expected type
            
        Raises:
            TypeValidationError: If type validation fails
            TypeConstraintError: If constraint validation fails
        """
        # Check if value is exactly the expected type (no inheritance)
        if type(value) != expected_type:
            raise TypeValidationError(
                f"Expected exact type {expected_type.__name__}, "
                f"got {type(value).__name__}"
            )
        
        # Check custom constraints if they exist
        if expected_type in self._constraints:
            if not self._constraints[expected_type](value):
                raise TypeConstraintError(
                    f"Value {value} failed constraint check for type {expected_type.__name__}"
                )

    def type_check(self, func: Callable) -> Callable:
        """
        Decorator that performs runtime type checking on function arguments and return value.
        
        Args:
            func: The function to decorate
            
        Returns:
            Decorated function with type checking
        """
        hints = get_type_hints(func)
        signature = inspect.signature(func)
        
        @wraps(func)
        def wrapper(*args, **kwargs):
            bound_arguments = signature.bind(*args, **kwargs)
            bound_arguments.apply_defaults()
            
            # Validate argument types
            for param_name, value in bound_arguments.arguments.items():
                if param_name in hints:
                    self.validate_type(value, hints[param_name])
            
            # Call the function
            result = func(*args, **kwargs)
            
            # Validate return type
            if 'return' in hints:
                self.validate_type(result, hints['return'])
            
            return result
            
        return wrapper

# Example usage
checker = TypeChecker()

# Register custom constraints
checker.register_constraint(int, lambda x: x >= 0)  # Only non-negative integers
checker.register_constraint(str, lambda x: len(x) <= 100)  # String length limit

# Example class with type checking
class SafeCalculator:
    @checker.type_check
    def add(self, a: int, b: int) -> int:
        return a + b
    
    @checker.type_check
    def process_text(self, text: str) -> str:
        return text.upper()

# Usage example
def demonstrate_type_checker():
    calc = SafeCalculator()
    
    try:
        # Valid operations
        result1 = calc.add(5, 3)  # Works fine
        print(f"5 + 3 = {result1}")
        
        result2 = calc.process_text("hello")  # Works fine
        print(f"Processed text: {result2}")
        
        # Invalid operations that will raise exceptions
        calc.add(5.0, 3)  # TypeError: float is not int
    except TypeValidationError as e:
        print(f"Type validation error: {e}")
    
    try:
        calc.add(-1, 5)  # Fails constraint check
    except TypeConstraintError as e:
        print(f"Constraint error: {e}")
        
    try:
        calc.process_text("x" * 101)  # Fails string length constraint
    except TypeConstraintError as e:
        print(f"Constraint error: {e}")

if __name__ == "__main__":
    demonstrate_type_checker()