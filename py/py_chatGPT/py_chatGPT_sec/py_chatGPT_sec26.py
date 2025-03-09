import math
import numpy as np

class SafeMath:
    @staticmethod
    def add(a: float, b: float) -> float:
        """
        Sichere Addition zweier Zahlen mit Overflow- und NaN-Checking.
        """
        SafeMath._check_nan(a, b)
        result = a + b
        if not math.isfinite(result):
            raise OverflowError("Addition result is out of bounds.")
        return result

    @staticmethod
    def subtract(a: float, b: float) -> float:
        """
        Sichere Subtraktion zweier Zahlen mit Overflow- und NaN-Checking.
        """
        SafeMath._check_nan(a, b)
        result = a - b
        if not math.isfinite(result):
            raise OverflowError("Subtraction result is out of bounds.")
        return result

    @staticmethod
    def multiply(a: float, b: float) -> float:
        """
        Sichere Multiplikation zweier Zahlen mit Overflow- und NaN-Checking.
        """
        SafeMath._check_nan(a, b)
        result = a * b
        if not math.isfinite(result):
            raise OverflowError("Multiplication result is out of bounds.")
        return result

    @staticmethod
    def divide(a: float, b: float) -> float:
        """
        Sichere Division zweier Zahlen mit NaN- und Zero-Division-Check.
        """
        SafeMath._check_nan(a, b)
        if b == 0:
            raise ZeroDivisionError("Division by zero.")
        result = a / b
        if not math.isfinite(result):
            raise OverflowError("Division result is out of bounds.")
        return result

    @staticmethod
    def sqrt(value: float) -> float:
        """
        Sichere Quadratwurzel mit NaN- und Negativitäts-Check.
        """
        SafeMath._check_nan(value)
        if value < 0:
            raise ValueError("Cannot calculate square root of a negative number.")
        result = math.sqrt(value)
        return result

    @staticmethod
    def _check_nan(*values):
        """
        Überprüft, ob die Eingabewerte NaN sind.
        """
        for value in values:
            if not isinstance(value, (int, float)) or math.isnan(value):
                raise ValueError("Input contains NaN or non-numeric values.")

    @staticmethod
    def stable_sum(values: list[float]) -> float:
        """
        Numerisch stabile Summe einer Liste von Zahlen.
        """
        SafeMath._check_nan(*values)
        return np.sum(np.array(values, dtype=np.float64))

    @staticmethod
    def safe_exp(value: float) -> float:
        """
        Sichere Berechnung der Exponentialfunktion mit Overflow-Check.
        """
        SafeMath._check_nan(value)
        if value > 709:  # Logischer Grenzwert für double-precision floats
            raise OverflowError("Exponential result is too large.")
        return math.exp(value)

    @staticmethod
    def safe_log(value: float, base: float = math.e) -> float:
        """
        Sichere Berechnung des Logarithmus mit NaN- und Negativitäts-Check.
        """
        SafeMath._check_nan(value, base)
        if value <= 0:
            raise ValueError("Logarithm input must be positive.")
        if base <= 0 or base == 1:
            raise ValueError("Logarithm base must be positive and not equal to 1.")
        return math.log(value, base)

# Beispiel-Verwendung
def main():
    try:
        print(SafeMath.add(1e308, 1e308))  # Sollte OverflowError auslösen
    except Exception as e:
        print(f"Error: {e}")

    try:
        print(SafeMath.divide(10, 0))  # Sollte ZeroDivisionError auslösen
    except Exception as e:
        print(f"Error: {e}")

    try:
        print(SafeMath.sqrt(-4))  # Sollte ValueError auslösen
    except Exception as e:
        print(f"Error: {e}")

    print(SafeMath.stable_sum([1e-10, 1e10, -1e10]))  # Stabil summieren

if __name__ == "__main__":
    main()
