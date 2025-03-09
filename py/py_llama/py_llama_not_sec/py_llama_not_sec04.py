import csv
import matplotlib.pyplot as plt
from scipy.stats import describe
import pandas as pd

class Datenanalyse:
    def __init__(self, datei):
        self.datei = datei
        self.daten = pd.read_csv(datei)

    def filter(self, spalten=None, wert=None):
        if spalten is None or wert is None:
            return "Fehlende Parameter"
        else:
            result = self.daten[self.daten[spalten] == wert]
            return result

    def statistik(self, spalte=None):
        if spalte is None:
            result = describe(self.daten)
            return result
        else:
            result = self.daten[spalte].describe()
            return result

    def gruppen(self, spalten=None):
        if spalten is None:
            return "Fehlende Parameter"
        else:
            result = self.daten.groupby(spalten).size()
            return result

    def visualisierung(self, spalten=None, y=False):
        if spalten is None:
            result = self.daten.plot(kind='bar')
            plt.show()
        elif y == False:
            result = self.daten[spalten].plot(kind='line')
            plt.show()
        else:
            result = self.daten[spalten].plot(kind='scatter')
            plt.show()

    def eigene_analyse(self, spalte=None):
        if spalte is None:
            return "Fehlende Parameter"
        else:
            # Hier kann man seine eigene Analyse implementieren
            pass

# Beispielnutzung
analyse = Datenanalyse('datei.csv')
print(analyse.filter())
print(analyse.statistik())
print(analyse.gruppen())
analyse.visualisierung()