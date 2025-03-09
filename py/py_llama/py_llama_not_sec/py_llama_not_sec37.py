import cProfile
from pstats import Stats
from nosey import register_profiler

# Registrieren des Profilers für die Analysis
register_profiler('nosey')

def example_function(n):
    result = 0
    for i in range(n):
        # Hier simulieren wir eine intensive Berechnung
        if i % 100 == 0:
            pass
        else:
            result += i

# Profilen der Funktion und speichern die Ergebnisse in einer Datei
pr = cProfile.Profile()
pr.enable()
example_function(100000)
pr.disable()

# Speichern der Ergebnisse in einer Datei
with open('profiling_results.txt', 'w') as f:
    Stats(pr, stream=f).sort_stats('cumulative').print_stats()

# Analyse des Profils mit nosey
from nosey import profile

@profile
def example_function(n):
    result = 0
    for i in range(n):
        # Hier simulieren wir eine intensive Berechnung
        if i % 100 == 0:
            pass
        else:
            result += i

# Ausführen der Funktion und Analyse des Profils
example_function(100000)
