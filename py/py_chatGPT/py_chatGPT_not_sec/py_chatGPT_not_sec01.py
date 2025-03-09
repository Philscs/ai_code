import math

class ScientificCalculator:
    def __init__(self):
        self.history = []

    def add(self, a, b):
        result = a + b
        self._add_to_history(f"{a} + {b} = {result}")
        return result

    def subtract(self, a, b):
        result = a - b
        self._add_to_history(f"{a} - {b} = {result}")
        return result

    def multiply(self, a, b):
        result = a * b
        self._add_to_history(f"{a} * {b} = {result}")
        return result

    def divide(self, a, b):
        if b == 0:
            self._add_to_history("Division durch Null ist nicht erlaubt.")
            raise ValueError("Division durch Null ist nicht erlaubt.")
        result = a / b
        self._add_to_history(f"{a} / {b} = {result}")
        return result

    def square_root(self, a):
        if a < 0:
            self._add_to_history("Wurzel aus einer negativen Zahl ist nicht definiert.")
            raise ValueError("Wurzel aus einer negativen Zahl ist nicht definiert.")
        result = math.sqrt(a)
        self._add_to_history(f"sqrt({a}) = {result}")
        return result

    def logarithm(self, a, base=math.e):
        if a <= 0:
            self._add_to_history("Logarithmus ist nur für positive Zahlen definiert.")
            raise ValueError("Logarithmus ist nur für positive Zahlen definiert.")
        result = math.log(a, base)
        self._add_to_history(f"log({a}, base={base}) = {result}")
        return result

    def sine(self, angle):
        result = math.sin(math.radians(angle))
        self._add_to_history(f"sin({angle}) = {result}")
        return result

    def cosine(self, angle):
        result = math.cos(math.radians(angle))
        self._add_to_history(f"cos({angle}) = {result}")
        return result

    def tangent(self, angle):
        if angle % 180 == 90:
            self._add_to_history("Tangens ist für diesen Winkel nicht definiert.")
            raise ValueError("Tangens ist für diesen Winkel nicht definiert.")
        result = math.tan(math.radians(angle))
        self._add_to_history(f"tan({angle}) = {result}")
        return result

    def _add_to_history(self, entry):
        self.history.append(entry)

    def show_history(self):
        return "\n".join(self.history)

    def clear_history(self):
        self.history = []

    def use_previous_result(self, index):
        if 0 <= index < len(self.history):
            entry = self.history[index]
            try:
                result = float(entry.split('=')[-1].strip())
                return result
            except ValueError:
                raise ValueError("Der ausgewählte Eintrag enthält kein numerisches Ergebnis.")
        else:
            raise IndexError("Ungültiger Index für Verlaufseintrag.")

if __name__ == "__main__":
    calculator = ScientificCalculator()

    while True:
        print("\nErweiterter Taschenrechner:")
        print("1. Addition")
        print("2. Subtraktion")
        print("3. Multiplikation")
        print("4. Division")
        print("5. Quadratwurzel")
        print("6. Logarithmus")
        print("7. Sinus")
        print("8. Kosinus")
        print("9. Tangens")
        print("10. Verlauf anzeigen")
        print("11. Verlauf löschen")
        print("12. Vorheriges Ergebnis verwenden")
        print("0. Beenden")

        choice = input("Wählen Sie eine Option: ")

        try:
            if choice == "0":
                break

            elif choice in {"1", "2", "3", "4"}:
                a = float(input("Erste Zahl: "))
                b = float(input("Zweite Zahl: "))

                if choice == "1":
                    print("Ergebnis:", calculator.add(a, b))
                elif choice == "2":
                    print("Ergebnis:", calculator.subtract(a, b))
                elif choice == "3":
                    print("Ergebnis:", calculator.multiply(a, b))
                elif choice == "4":
                    print("Ergebnis:", calculator.divide(a, b))

            elif choice == "5":
                a = float(input("Zahl: "))
                print("Ergebnis:", calculator.square_root(a))

            elif choice == "6":
                a = float(input("Zahl: "))
                base = input("Basis (leer lassen für e): ")
                base = float(base) if base else math.e
                print("Ergebnis:", calculator.logarithm(a, base))

            elif choice == "7":
                angle = float(input("Winkel in Grad: "))
                print("Ergebnis:", calculator.sine(angle))

            elif choice == "8":
                angle = float(input("Winkel in Grad: "))
                print("Ergebnis:", calculator.cosine(angle))

            elif choice == "9":
                angle = float(input("Winkel in Grad: "))
                print("Ergebnis:", calculator.tangent(angle))

            elif choice == "10":
                print("Verlauf:")
                print(calculator.show_history())

            elif choice == "11":
                calculator.clear_history()
                print("Verlauf wurde gelöscht.")

            elif choice == "12":
                index = int(input("Index des vorherigen Ergebnisses: "))
                result = calculator.use_previous_result(index)
                print("Verwendetes Ergebnis:", result)

            else:
                print("Ungültige Auswahl. Bitte versuchen Sie es erneut.")

        except Exception as e:
            print(f"Fehler: {e}")
