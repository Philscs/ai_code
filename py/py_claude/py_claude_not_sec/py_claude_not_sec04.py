import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from typing import Callable, List, Dict, Any, Union
from dataclasses import dataclass

@dataclass
class AnalysisConfig:
    """Konfiguration für Analyseoperationen"""
    group_by: List[str] = None
    filters: Dict[str, Callable] = None
    custom_functions: Dict[str, Callable] = None

class CSVAnalyzer:
    def __init__(self, filepath: str):
        """
        Initialisiert den CSV-Analyzer
        
        Args:
            filepath: Pfad zur CSV-Datei
        """
        self.data = pd.read_csv(filepath)
        self.original_data = self.data.copy()
        self._custom_functions = {}
        
    def reset_data(self):
        """Setzt die Daten auf den ursprünglichen Zustand zurück"""
        self.data = self.original_data.copy()
        
    def add_custom_function(self, name: str, func: Callable):
        """
        Fügt eine benutzerdefinierte Analysefunktion hinzu
        
        Args:
            name: Name der Funktion
            func: Die Analysefunktion
        """
        self._custom_functions[name] = func
        
    def apply_filter(self, column: str, condition: Callable):
        """
        Wendet einen Filter auf eine Spalte an
        
        Args:
            column: Spaltenname
            condition: Filterfunktion die True/False zurückgibt
        """
        self.data = self.data[condition(self.data[column])]
        
    def group_and_analyze(self, 
                         group_columns: List[str], 
                         agg_functions: Dict[str, Union[str, Callable]]):
        """
        Gruppiert Daten und wendet Aggregationsfunktionen an
        
        Args:
            group_columns: Spalten für Gruppierung
            agg_functions: Dict mit Spalten und Aggregationsfunktionen
        
        Returns:
            DataFrame mit gruppierten Ergebnissen
        """
        return self.data.groupby(group_columns).agg(agg_functions)
    
    def calculate_statistics(self, columns: List[str] = None):
        """
        Berechnet grundlegende statistische Kennzahlen
        
        Args:
            columns: Optionale Liste von Spalten (default: alle numerischen)
            
        Returns:
            DataFrame mit statistischen Kennzahlen
        """
        if columns is None:
            columns = self.data.select_dtypes(include=[np.number]).columns
        
        stats = {}
        for col in columns:
            stats[col] = {
                'mean': self.data[col].mean(),
                'median': self.data[col].median(),
                'std': self.data[col].std(),
                'min': self.data[col].min(),
                'max': self.data[col].max(),
                'q25': self.data[col].quantile(0.25),
                'q75': self.data[col].quantile(0.75)
            }
        return pd.DataFrame(stats)
    
    def plot_distribution(self, column: str, plot_type: str = 'histogram'):
        """
        Visualisiert die Verteilung einer Spalte
        
        Args:
            column: Zu visualisierende Spalte
            plot_type: Art der Visualisierung ('histogram', 'boxplot', 'violin')
        """
        plt.figure(figsize=(10, 6))
        
        if plot_type == 'histogram':
            sns.histplot(data=self.data, x=column, kde=True)
        elif plot_type == 'boxplot':
            sns.boxplot(data=self.data, y=column)
        elif plot_type == 'violin':
            sns.violinplot(data=self.data, y=column)
        
        plt.title(f'Verteilung von {column}')
        plt.show()
        
    def plot_correlation_matrix(self, columns: List[str] = None):
        """
        Erstellt eine Korrelationsmatrix
        
        Args:
            columns: Optionale Liste von Spalten (default: alle numerischen)
        """
        if columns is None:
            numeric_data = self.data.select_dtypes(include=[np.number])
        else:
            numeric_data = self.data[columns]
            
        corr_matrix = numeric_data.corr()
        
        plt.figure(figsize=(10, 8))
        sns.heatmap(corr_matrix, annot=True, cmap='coolwarm', center=0)
        plt.title('Korrelationsmatrix')
        plt.show()
        
    def apply_custom_analysis(self, function_name: str, *args, **kwargs):
        """
        Führt eine benutzerdefinierte Analysefunktion aus
        
        Args:
            function_name: Name der benutzerdefinierten Funktion
            *args, **kwargs: Argumente für die Funktion
            
        Returns:
            Ergebnis der benutzerdefinierten Funktion
        """
        if function_name not in self._custom_functions:
            raise KeyError(f"Funktion '{function_name}' nicht gefunden")
        
        return self._custom_functions[function_name](self.data, *args, **kwargs)

# Beispiel für die Verwendung:
def example_usage():
    # Analyzer erstellen
    analyzer = CSVAnalyzer('daten.csv')
    
    # Benutzerdefinierte Funktion hinzufügen
    def calculate_growth_rate(data, column):
        return (data[column].pct_change() * 100).mean()
    
    analyzer.add_custom_function('growth_rate', calculate_growth_rate)
    
    # Filter anwenden
    analyzer.apply_filter('price', lambda x: x > 100)
    
    # Gruppierung und Aggregation
    results = analyzer.group_and_analyze(
        ['category'], 
        {'price': 'mean', 'quantity': 'sum'}
    )
    
    # Statistiken berechnen
    stats = analyzer.calculate_statistics(['price', 'quantity'])
    
    # Visualisierungen
    analyzer.plot_distribution('price', 'histogram')
    analyzer.plot_correlation_matrix()
    
    # Benutzerdefinierte Analyse ausführen
    growth = analyzer.apply_custom_analysis('growth_rate', 'price')
    
    return results, stats, growth

if __name__ == "__main__":
    results, stats, growth = example_usage()