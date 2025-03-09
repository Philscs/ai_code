import numpy as np

class SafeMath:
    def __init__(self):
        self._overflow_threshold = 1e18
        self._underflow_threshold = 1e-300

    def add(self, a, b):
        """
        Add two numbers with overflow checking.

        Args:
            a (float): Das erste Zahl.
            b (float): Das zweite Zahl.

        Returns:
            float: Die Summe der beiden Zahlen.
        """
        if not np.isfinite(a) or not np.isfinite(b):
            raise ValueError("Beide Zahlen müssen reell sein.")

        # Überprüfen, ob die Summe den overflow-Überschreitungspunkt erreicht hat
        if a + b > self._overflow_threshold:
            raise OverflowError("Overflow: Die Summe ist zu groß.")

        # Überprüfen, ob die Summe den unterflow-Unterschreitenwert erreicht hat
        if a + b < self._underflow_threshold:
            raise UnderflowError("Underflow: Die Summe ist zu klein.")

        return a + b

    def subtract(self, a, b):
        """
        Subtrahieren zwei Zahlen mit overflow checking.

        Args:
            a (float): Das erste Zahl.
            b (float): Das zweite Zahl.

        Returns:
            float: Die Differenz der beiden Zahlen.
        """
        if not np.isfinite(a) or not np.isfinite(b):
            raise ValueError("Beide Zahlen müssen reell sein.")

        # Überprüfen, ob die Differenz den overflow-Überschreitungspunkt erreicht hat
        if a - b > self._overflow_threshold:
            raise OverflowError("Overflow: Die Differenz ist zu groß.")

        # Überprüfen, ob die Differenz den unterflow-Unterschreitenwert erreicht hat
        if a - b < self._underflow_threshold:
            raise UnderflowError("Underflow: Die Differenz ist zu klein.")

        return a - b

    def multiply(self, a, b):
        """
        Multiplizieren zwei Zahlen mit overflow checking.

        Args:
            a (float): Das erste Zahl.
            b (float): Das zweite Zahl.

        Returns:
            float: Das Produkt der beiden Zahlen.
        """
        if not np.isfinite(a) or not np.isfinite(b):
            raise ValueError("Beide Zahlen müssen reell sein.")

        # Überprüfen, ob das Produkt den overflow-Überschreitungspunkt erreicht hat
        if a * b > self._overflow_threshold:
            raise OverflowError("Overflow: Das Produkt ist zu groß.")

        return a * b

    def divide(self, a, b):
        """
        Dividieren zwei Zahlen mit overflow checking.

        Args:
            a (float): Das erste Zahl.
            b (float): Das zweite Zahl.

        Returns:
            float: Die Division der beiden Zahlen.
        """
        if not np.isfinite(a) or not np.isfinite(b):
            raise ValueError("Beide Zahlen müssen reell sein.")

        # Überprüfen, ob die Division den unterflow-Unterschreitenwert erreicht hat
        if a / b < self._underflow_threshold:
            raise UnderflowError("Underflow: Die Division ist zu klein.")

        return a / b

def is_safe_number(num):
    """
    Überprüfen, ob eine Zahl sicher ist.

    Args:
        num (float): Die Zahl zu überprüfen.

    Returns:
        bool: Ob die Zahl sicher ist.
    """
    if not np.isfinite(num):
        return False

    # Overflow-Überschreitungspunkt
    overflow_threshold = 1e18

    # Unterflow-Unterschreitenwert
    underflow_threshold = 1e-300

    if num < -overflow_threshold or num > overflow_threshold:
        return False

    if num < underflow_threshold:
        return False

    return True