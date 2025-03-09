import math

def safe_math_calculation(a, b, operator):
    try:
        # Check for NaN values
        if math.isnan(a) or math.isnan(b):
            return math.nan

        # Check for overflow
        if math.isinf(a) or math.isinf(b):
            return math.inf

        # Perform the calculation based on the operator
        if operator == '+':
            result = a + b
        elif operator == '-':
            result = a - b
        elif operator == '*':
            result = a * b
        elif operator == '/':
            result = a / b
        elif operator == '%':
            result = a % b
        elif operator == '**':
            result = a ** b
        else:
            raise ValueError("Invalid operator")

        # Check for overflow again
        if math.isinf(result):
            return math.inf

        return result

    except Exception as e:
        print(f"Error: {e}")
        return None
