import math
import sys
from decimal import Decimal, getcontext
from typing import Union, Optional, Tuple
import numpy as np

class SecureMathError(Exception):
    """Base exception class for secure math operations."""
    pass

class OverflowError(SecureMathError):
    """Raised when an operation would result in overflow."""
    pass

class UnderflowError(SecureMathError):
    """Raised when an operation would result in underflow."""
    pass

class NumericalInstabilityError(SecureMathError):
    """Raised when numerical instability is detected."""
    pass

class SecureMath:
    def __init__(self, precision: int = 53):
        """
        Initialize SecureMath with specified precision.
        
        Args:
            precision: Number of bits for floating-point precision (default: 53 for double precision)
        """
        self.precision = precision
        getcontext().prec = precision
        self.max_float = sys.float_info.max
        self.min_float = sys.float_info.min
        
    def _check_nan(self, value: float) -> None:
        """Check if value is NaN and raise exception if true."""
        if math.isnan(value):
            raise ValueError("NaN value detected")

    def _check_overflow(self, value: float) -> None:
        """Check if value would cause overflow."""
        if abs(value) > self.max_float:
            raise OverflowError(f"Value {value} exceeds maximum float value")

    def _check_underflow(self, value: float) -> None:
        """Check if value would cause underflow."""
        if abs(value) < self.min_float and value != 0:
            raise UnderflowError(f"Value {value} is below minimum float value")

    def add(self, a: float, b: float) -> float:
        """
        Securely add two numbers with overflow checking.
        """
        self._check_nan(a)
        self._check_nan(b)
        
        # Use Decimal for precise calculation
        result = float(Decimal(str(a)) + Decimal(str(b)))
        
        self._check_overflow(result)
        return result

    def multiply(self, a: float, b: float) -> float:
        """
        Securely multiply two numbers with overflow checking.
        """
        self._check_nan(a)
        self._check_nan(b)
        
        # Use Decimal for precise calculation
        result = float(Decimal(str(a)) * Decimal(str(b)))
        
        self._check_overflow(result)
        self._check_underflow(result)
        return result

    def divide(self, a: float, b: float, tolerance: float = 1e-10) -> float:
        """
        Securely divide two numbers with checks for division by zero and small denominators.
        """
        self._check_nan(a)
        self._check_nan(b)
        
        if abs(b) < tolerance:
            raise ValueError(f"Denominator {b} is too close to zero")
        
        # Use Decimal for precise calculation
        result = float(Decimal(str(a)) / Decimal(str(b)))
        
        self._check_overflow(result)
        self._check_underflow(result)
        return result

    def sqrt(self, x: float) -> float:
        """
        Securely calculate square root with domain checking.
        """
        self._check_nan(x)
        
        if x < 0:
            raise ValueError("Square root of negative number")
        
        # Use Decimal for precise calculation
        result = float(Decimal(str(x)).sqrt())
        
        self._check_underflow(result)
        return result

    def exp(self, x: float) -> float:
        """
        Securely calculate exponential with overflow checking.
        """
        self._check_nan(x)
        
        # Check if result would overflow before calculation
        if x > math.log(self.max_float):
            raise OverflowError(f"exp({x}) would overflow")
        
        result = math.exp(x)
        self._check_overflow(result)
        return result

    def sum_array(self, arr: list[float], stable: bool = True) -> float:
        """
        Securely sum an array of numbers with optional Kahan summation for stability.
        """
        if not arr:
            return 0.0
            
        for x in arr:
            self._check_nan(x)
            
        if stable:
            # Kahan summation for numerical stability
            s = 0.0
            c = 0.0  # compensation term
            
            for x in arr:
                y = x - c
                t = s + y
                c = (t - s) - y
                s = t
                
                self._check_overflow(s)
            
            return s
        else:
            # Simple sum with overflow checking
            result = sum(arr)
            self._check_overflow(result)
            return result

    def dot_product(self, a: list[float], b: list[float], stable: bool = True) -> float:
        """
        Securely calculate dot product with stability checks.
        """
        if len(a) != len(b):
            raise ValueError("Vectors must have same length")
            
        products = [self.multiply(x, y) for x, y in zip(a, b)]
        return self.sum_array(products, stable=stable)

# Example usage and testing
def test_secure_math():
    sm = SecureMath()
    
    # Basic operations
    assert sm.add(1.5, 2.5) == 4.0
    assert sm.multiply(2.0, 3.0) == 6.0
    assert abs(sm.divide(5.0, 2.0) - 2.5) < 1e-10
    assert abs(sm.sqrt(16.0) - 4.0) < 1e-10
    
    # Test overflow protection
    try:
        sm.multiply(1e308, 1e308)
        assert False, "Should have raised OverflowError"
    except OverflowError:
        pass
    
    # Test division by zero protection
    try:
        sm.divide(1.0, 0.0)
        assert False, "Should have raised ValueError"
    except ValueError:
        pass
    
    # Test array operations
    arr = [1.0, 2.0, 3.0, 4.0, 5.0]
    assert abs(sm.sum_array(arr) - 15.0) < 1e-10
    
    # Test dot product
    a = [1.0, 2.0, 3.0]
    b = [4.0, 5.0, 6.0]
    assert abs(sm.dot_product(a, b) - 32.0) < 1e-10

if __name__ == "__main__":
    test_secure_math()