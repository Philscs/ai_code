import math
import re

class WissenschaftlicherRechner:
    def __init__(self):
        self.historie = []
        self.letzte_ergebnisse = []
    
    def berechne(self, ausdruck):
        try:
            # Ersetze 'ANS' durch das letzte Ergebnis
            while 'ANS' in ausdruck.upper():
                if not self.letzte_ergebnisse:
                    raise ValueError("Keine vorherigen Ergebnisse verfügbar")
                ausdruck = ausdruck.upper().replace('ANS', str(self.letzte_ergebnisse[-1]))
            
            # Ersetze mathematische Funktionen
            ausdruck = ausdruck.lower()
            ausdruck = ausdruck.replace('sin(', 'math.sin(')
            ausdruck = ausdruck.replace('cos(', 'math.cos(')
            ausdruck = ausdruck.replace('tan(', 'math.tan(')
            ausdruck = ausdruck.replace('sqrt(', 'math.sqrt(')
            ausdruck = ausdruck.replace('log(', 'math.log10(')
            ausdruck = ausdruck.replace('ln(', 'math.log(')
            ausdruck = ausdruck.replace('pi', str(math.pi))
            ausdruck = ausdruck.replace('e', str(math.e))
            
            # Berechne das Ergebnis
            ergebnis = eval(ausdruck)
            
            # Speichere in Historie und Ergebnisliste
            self.historie.append(f"{ausdruck} = {ergebnis}")
            self.letzte_ergebnisse.append(ergebnis)
            
            return ergebnis
            
        except Exception as e:
            return f"Fehler: {str(e)}"
    
    def zeige_historie(self):
        print("\nBerechnungshistorie:")
        for i, eintrag in enumerate(self.historie, 1):
            print(f"{i}. {eintrag}")
    
    def zeige_hilfe(self):
        print("""
Verfügbare Operationen:
- Grundrechenarten: +, -, *, /, ** (Potenz)
- Wissenschaftliche Funktionen:
  * sin(x), cos(x), tan(x) - Trigonometrische Funktionen
  * sqrt(x) - Quadratwurzel
  * log(x) - Logarithmus zur Basis 10
  * ln(x) - Natürlicher Logarithmus
- Konstanten:
  * pi - Kreiszahl π
  * e - Eulersche Zahl
- Spezielle Befehle:
  * ANS - Verwendet das letzte Ergebnis
  * historie - Zeigt die Berechnungshistorie
  * hilfe - Zeigt diese Hilfe
  * ende - Beendet das Programm
""")

def main():
    rechner = WissenschaftlicherRechner()
    print("Wissenschaftlicher Taschenrechner")
    print("Geben Sie 'hilfe' ein für eine Liste der verfügbaren Operationen.")
    
    while True:
        eingabe = input("\nGeben Sie einen Ausdruck ein: ").strip()
        
        if eingabe.lower() == 'ende':
            print("Programm wird beendet.")
            break
        elif eingabe.lower() == 'historie':
            rechner.zeige_historie()
        elif eingabe.lower() == 'hilfe':
            rechner.zeige_hilfe()
        else:
            ergebnis = rechner.berechne(eingabe)
            print(f"Ergebnis: {ergebnis}")

if __name__ == "__main__":
    main()