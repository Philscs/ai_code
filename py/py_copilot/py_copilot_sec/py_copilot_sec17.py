from typing import Any, Dict, Type, get_type_hints
import json
from dataclasses import dataclass

class ValidationError(Exception):
    """Custom exception for validation errors."""
    pass

class SecureAttributeMetaclass(type):
    """
    A metaclass for secure attribute validation with type checking
    and protection against code injection.
    """
    
    def __new__(mcs, name: str, bases: tuple, namespace: dict) -> Type:
        # Collect all type annotations
        type_hints = get_type_hints(namespace.get('__annotations__', {}))
        
        # Validate existing class attributes
        for attr_name, attr_value in namespace.items():
            if attr_name in type_hints:
                mcs._validate_attribute(attr_name, attr_value, type_hints[attr_name])
        
        # Create property wrappers for all typed attributes
        for attr_name, attr_type in type_hints.items():
            if attr_name not in namespace:
                namespace[attr_name] = None
            
            # Create property with getter and setter
            namespace[attr_name] = mcs._create_validated_property(
                attr_name, 
                attr_type,
                namespace.get(attr_name)
            )
            
        # Implement secure serialization
        def to_json(self) -> str:
            """Secure JSON serialization of the instance."""
            data = {
                attr: getattr(self, attr)
                for attr in type_hints
                if hasattr(self, attr)
            }
            return json.dumps(data, default=str)
        
        namespace['to_json'] = to_json
        
        # Implement custom __setattr__ for additional security
        def __setattr__(self, name: str, value: Any) -> None:
            """
            Override __setattr__ to validate dynamic attribute assignments.
            """
            # Check if the attribute is allowed
            if name not in type_hints and not name.startswith('_'):
                raise ValidationError(
                    f"Dynamic assignment of '{name}' is not allowed"
                )
            
            # Validate the value if a type hint exists
            if name in type_hints:
                mcs._validate_attribute(name, value, type_hints[name])
            
            # Set the attribute
            super().__setattr__(name, value)
            
        namespace['__setattr__'] = __setattr__
        
        return super().__new__(mcs, name, bases, namespace)
    
    @staticmethod
    def _validate_attribute(name: str, value: Any, expected_type: Type) -> None:
        """
        Validate an attribute against its expected type and perform
        additional security checks.
        """
        # Check for None values
        if value is None:
            if getattr(expected_type, '__origin__', None) is not type(None):
                raise ValidationError(f"Attribute '{name}' cannot be None")
            return
            
        # Check the type
        if not isinstance(value, expected_type):
            raise ValidationError(
                f"Attribute '{name}' must be of type {expected_type}, "
                f"but is {type(value)}"
            )
            
        # Additional security checks for strings
        if isinstance(value, str):
            # Check for potentially dangerous strings
            dangerous_patterns = ['__', 'eval', 'exec', 'import']
            if any(pattern in value for pattern in dangerous_patterns):
                raise ValidationError(
                    f"Attribute '{name}' contains potentially unsafe string"
                )
    
    @staticmethod
    def _create_validated_property(
        attr_name: str,
        attr_type: Type,
        default_value: Any = None
    ) -> property:
        """
        Create a property with validation for a typed attribute.
        """
        storage_name = f'_{attr_name}'
        
        def getter(instance: Any) -> Any:
            return getattr(instance, storage_name, default_value)
            
        def setter(instance: Any, value: Any) -> None:
            SecureAttributeMetaclass._validate_attribute(
                attr_name, value, attr_type
            )
            setattr(instance, storage_name, value)
            
        return property(getter, setter)

# Example usage
@dataclass
class Person(metaclass=SecureAttributeMetaclass):
    name: str
    age: int
    email: str = None

# Example class with invariants
class BankAccount(metaclass=SecureAttributeMetaclass):
    balance: float
    account_number: str
    
    def __init__(self, account_number: str, initial_balance: float = 0.0):
        self.account_number = account_number
        self.balance = initial_balance
    
    def withdraw(self, amount: float) -> None:
        if amount > self.balance:
            raise ValueError("Insufficient balance")
        self.balance -= amount
    
    def deposit(self, amount: float) -> None:
        if amount <= 0:
            raise ValueError("Deposit amount must be positive")
        self.balance += amount
