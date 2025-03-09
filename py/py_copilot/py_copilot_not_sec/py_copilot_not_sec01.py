class ScientificCalculator:
    def __init__(self):
        self.history = []

    def add(self, num1, num2):
        result = num1 + num2
        self.history.append(f"Addition: {num1} + {num2} = {result}")
        return result

    def subtract(self, num1, num2):
        result = num1 - num2
        self.history.append(f"Subtraction: {num1} - {num2} = {result}")
        return result

    def multiply(self, num1, num2):
        result = num1 * num2
        self.history.append(f"Multiplication: {num1} * {num2} = {result}")
        return result

    def divide(self, num1, num2):
        if num2 == 0:
            raise ValueError("Cannot divide by zero")
        result = num1 / num2
        self.history.append(f"Division: {num1} / {num2} = {result}")
        return result

    def square_root(self, num):
        result = num ** 0.5
        self.history.append(f"Square Root: sqrt({num}) = {result}")
        return result

    def logarithm(self, num):
        import math
        result = math.log(num)
        self.history.append(f"Logarithm: log({num}) = {result}")
        return result

    def sine(self, angle):
        import math
        result = math.sin(math.radians(angle))
        self.history.append(f"Sine: sin({angle}) = {result}")
        return result

    def cosine(self, angle):
        import math
        result = math.cos(math.radians(angle))
        self.history.append(f"Cosine: cos({angle}) = {result}")
        return result

    def tangent(self, angle):
        import math
        result = math.tan(math.radians(angle))
        self.history.append(f"Tangent: tan({angle}) = {result}")
        return result

    def get_history(self):
        return self.history

class Calculator(ScientificCalculator):
    def __init__(self):
        super().__init__()

    def calculate(self, expression):
        try:
            result = eval(expression)
            self.history.append(f"Calculation: {expression} = {result}")
            return result
        except Exception as e:
            raise ValueError(f"Invalid expression: {expression}") from e
