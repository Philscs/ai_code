import networkx as nx
import matplotlib.pyplot as plt

class ProgrammAbhangigkeit:
    def __init__(self):
        self.abhangigkeiten = {}
        self.graph = nx.DiGraph()

    def add_abhängigkeit(self, programm, abhängiges_programm):
        """Fügt einer Abhängigkeit hinzu"""
        if programm not in self.abhangigkeiten:
            self.abhangigkeiten[programm] = []
        self.abhangigkeiten[programm].append(abhängiges_programm)
        self.graph.add_edge(programm, abhängiges_programm)

    def visualisierung(self):
        """Visualisiert die Abhängigkeiten"""
        pos = nx.spring_layout(self.graph)
        nx.draw_networkx(self.graph, pos)
        plt.show()

    def impact_analyse(self, programm, wunsch_größe):
        """Berechnet den Wunsch nach einer bestimmten Größe von Abhängigkeiten für ein 
Programm."""
        grösste_abhängigkeit = 0
        abhängigkeitsgröße = 0
        zu_hoch = False

        for abhängiges_programm in self.abhangigkeiten[programm]:
            abhängigkeitsgröße += len(self.abhangigkeiten[abhängiges_programm])

            if abhängigkeitsgröße >= wunsch_größe:
                zu_hoch = True
                break

        for abhängiges_programm in self.abhangigkeiten[programm]:
            grösste_abhängigkeit += len(self.abhangigkeiten[abhängiges_programm])

        if zu_hoch:
            print("Die Abhängigkeit ist zu groß!")
        else:
            print(f"Die Gesamtzahl der Abhängigkeiten für {programm} beträgt: 
{abhängigkeitsgröße}")
            print(f"Grosseste Abhängigkeit für {programm}: {grösste_abhängigkeit}")


# Beispiel usage
abhangigkeitssystem = ProgrammAbhangigkeit()

abhangigkeitssystem.add_abhängigkeit("A", "B")
abhangigkeitssystem.add_abhängigkeit("A", "C")
abhangigkeitssystem.add_abhängigkeit("B", "D")

print("Visualisierung:")
abhangigkeitssystem.visualisierung()

print("\nImpact-Analyse:")
abhangigkeitssystem.impact_analyse("A", 10)