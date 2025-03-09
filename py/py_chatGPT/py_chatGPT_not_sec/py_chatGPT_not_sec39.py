import subprocess
import json
import os

def run_pylint(file_path):
    """Führt pylint für statische Analyse aus."""
    try:
        result = subprocess.run(
            ["pylint", file_path, "--output-format=json"],
            capture_output=True,
            text=True
        )
        return json.loads(result.stdout)
    except json.JSONDecodeError:
        print("Fehler beim Verarbeiten des Pylint-Ergebnisses.")
        return []

def run_flake8(file_path):
    """Führt flake8 für Style-Checking aus."""
    try:
        result = subprocess.run(
            ["flake8", file_path, "--format=json"],
            capture_output=True,
            text=True
        )
        return json.loads(result.stdout)
    except json.JSONDecodeError:
        print("Fehler beim Verarbeiten des Flake8-Ergebnisses.")
        return {}

def analyze_code(file_path):
    """Analysiert den angegebenen Code mit pylint und flake8."""
    if not os.path.exists(file_path):
        print(f"Die Datei {file_path} existiert nicht.")
        return

    print(f"Analysiere Datei: {file_path}\n")

    print("-- Pylint Analyse --")
    pylint_results = run_pylint(file_path)
    for issue in pylint_results:
        print(f"{issue['type'].capitalize()}: {issue['message']} (Zeile {issue['line']})")

    print("\n-- Flake8 Analyse --")
    flake8_results = run_flake8(file_path)
    for file, issues in flake8_results.items():
        for issue in issues:
            print(f"{issue['code']}: {issue['text']} (Zeile {issue['line']}, Spalte {issue['column']})")

if __name__ == "__main__":
    print("Automatisches Code-Review-Tool")
    file_path = input("Geben Sie den Pfad zur zu analysierenden Datei ein: ")
    analyze_code(file_path)
