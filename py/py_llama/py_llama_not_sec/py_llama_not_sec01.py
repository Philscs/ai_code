import math

class Taschenrechner:
    def __init__(self):
        self.verbefuehrung = {
            "1": "Addition",
            "2": "Subtraktion",
            "3": "Multiplikation",
            "4": "Dividion",
            "5": "Wurzel",
            "6": "Logarithmus",
            "7": "Trigonometrische Funktionen"
        }
        self.gebrauchene_berechnungen = []

    def anfangsdaten(self):
        print("Willkommen im Taschenrechner!")
        print("Es gibt verschiedene Berechnungsarten verfügbar:")
        for key in self.verbefuehrung:
            print(key, " - ", self.verbefuehrung[key])

    def berechne_addition(self):
        num1 = float(input("Geben Sie den ersten Wert ein: "))
        num2 = float(input("Geben Sie den zweiten Wert ein: "))
        return num1 + num2

    def berechne_subtraktion(self):
        num1 = float(input("Geben Sie den ersten Wert ein: "))
        num2 = float(input("Geben Sie den zweiten Wert ein: "))
        return num1 - num2

    def berechne_multiplikation(self):
        num1 = float(input("Geben Sie den ersten Wert ein: "))
        num2 = float(input("Geben Sie den zweiten Wert ein: "))
        return num1 * num2

    def berechne_division(self):
        num1 = float(input("Geben Sie den ersten Wert ein: "))
        num2 = float(input("Geben Sie den zweiten Wert ein: "))
        if num2 == 0:
            print("Fehler: Division durch Null nicht erlaubt")
            return None
        return num1 / num2

    def berechne_wurzel(self):
        num = float(input("Geben Sie den Wert ein: "))
        if num < 0:
            print("Fehler: Wurzeln sind nur für positive Zahlen gültig")
            return None
        return math.sqrt(num)

    def berechne_logarithmus(self):
        num = float(input("Geben Sie den Wert ein: "))
        if num <= 0:
            print("Fehler: Logarithmus von nicht positiven Zahlen nicht gültig")
            return None
        return math.log10(num)

    def berechne_trigonometrische_funktion(self):
        num = float(input("Geben Sie den Wert ein: "))
        ergebnis = round(math.sin(num), 2)
        print(ergebnis)

    def main_menu(self):
        while True:
            self.anfangsdaten()
            print("\nBerechnungsarten:")
            print("1 - Addition")
            print("2 - Subtraktion")
            print("3 - Multiplikation")
            print("4 - Division")
            print("5 - Wurzel")
            print("6 - Logarithmus")
            print("7 - Trigonometrische Funktionen")
            print("8 - Historie anzeigen")

            ergebnis = input("\nGeben Sie eine Zahl ein (1-8): ")

            if ergebnis == "8":
                self.anzeigen_historie()
                continue
            elif ergebnis not in [str(x) for x in range(1, 9)]:
                print("Fehler: Ungültige Auswahl")
                continue

            berechnung = getattr(self, 'berechne_' + ergebnis)
            if berechnung is None:
                print("Fehler: Berechnungsfehler")
                continue
            self.gebrauchene_berechnungen.append(ergebnis)

            result = berechnung()
            if result is not None:
                self.gebrauchene_berechnungen.append(result)
                print(f"Ergebnis: {result}")
            else:
                print("Fehler")

    def anzeigen_historie(self):
        for i in range(len(self.gebrauchene_berechnungen) - 1, -1, -1):
            print(f"{self.gebrauchene_berechnungen[i]} = {self.gebrauchene_berechnungen[i + 1]}")


# Beispiel usage
if __name__ == "__main__":
    Taschenrechner().main_menu()
