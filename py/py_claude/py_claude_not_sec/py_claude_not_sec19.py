import numpy as np
import pandas as pd
from sklearn.preprocessing import StandardScaler
from sklearn.ensemble import IsolationForest
from statsmodels.tsa.seasonal import seasonal_decompose
from statsmodels.tsa.holtwinters import ExponentialSmoothing
from statsmodels.tsa.arima.model import ARIMA
from scipy import stats

class TimeSeriesAnalyzer:
    def __init__(self, data, date_column='date', value_column='value'):
        """
        Initialisiert den TimeSeriesAnalyzer.
        
        Args:
            data (pd.DataFrame): DataFrame mit Zeitreihendaten
            date_column (str): Name der Datumsspalte
            value_column (str): Name der Wertspalte
        """
        self.data = data.copy()
        self.date_column = date_column
        self.value_column = value_column
        
        # Stellt sicher, dass das Datumsformat korrekt ist
        self.data[date_column] = pd.to_datetime(self.data[date_column])
        self.data = self.data.sort_values(date_column)
        
        # Setzt das Datum als Index
        self.data.set_index(date_column, inplace=True)

    def decompose_series(self, period=None):
        """
        Zerlegt die Zeitreihe in Trend, Saisonalität und Residuen.
        
        Args:
            period (int): Periodenlänge für saisonale Zerlegung
            
        Returns:
            dict: Komponenten der Zerlegung
        """
        if period is None:
            # Versucht die Periode automatisch zu bestimmen
            if len(self.data) >= 730:  # 2 Jahre täglich
                period = 365
            elif len(self.data) >= 104:  # 2 Jahre wöchentlich
                period = 52
            elif len(self.data) >= 24:  # 2 Jahre monatlich
                period = 12
            else:
                period = len(self.data) // 2

        decomposition = seasonal_decompose(
            self.data[self.value_column], 
            period=period, 
            extrapolate_trend='freq'
        )
        
        return {
            'trend': decomposition.trend,
            'seasonal': decomposition.seasonal,
            'residual': decomposition.resid
        }

    def detect_anomalies(self, contamination=0.1):
        """
        Erkennt Anomalien in der Zeitreihe mittels Isolation Forest.
        
        Args:
            contamination (float): Erwarteter Anteil der Anomalien
            
        Returns:
            pd.Series: Boolean-Serie mit markierten Anomalien
        """
        # Daten für Isolation Forest vorbereiten
        scaler = StandardScaler()
        scaled_data = scaler.fit_transform(self.data[[self.value_column]])
        
        # Isolation Forest trainieren
        iso_forest = IsolationForest(
            contamination=contamination,
            random_state=42
        )
        
        # Anomalien vorhersagen (-1 für Anomalien, 1 für normale Datenpunkte)
        predictions = iso_forest.fit_predict(scaled_data)
        
        return pd.Series(predictions == -1, index=self.data.index)

    def forecast_arima(self, steps=30, order=(1,1,1)):
        """
        Erstellt eine ARIMA-Vorhersage.
        
        Args:
            steps (int): Anzahl der vorherzusagenden Zeitschritte
            order (tuple): ARIMA-Order (p,d,q)
            
        Returns:
            pd.Series: Vorhersagewerte
        """
        model = ARIMA(self.data[self.value_column], order=order)
        results = model.fit()
        forecast = results.forecast(steps=steps)
        return forecast

    def forecast_exponential_smoothing(self, steps=30, seasonal_periods=None):
        """
        Erstellt eine Vorhersage mittels exponentieller Glättung.
        
        Args:
            steps (int): Anzahl der vorherzusagenden Zeitschritte
            seasonal_periods (int): Länge der saisonalen Periode
            
        Returns:
            pd.Series: Vorhersagewerte
        """
        model = ExponentialSmoothing(
            self.data[self.value_column],
            seasonal_periods=seasonal_periods,
            seasonal='add' if seasonal_periods else None
        )
        results = model.fit()
        forecast = results.forecast(steps)
        return forecast

    def analyze_trend(self, window=30):
        """
        Analysiert den Trend mittels gleitendem Durchschnitt und Mann-Kendall-Test.
        
        Args:
            window (int): Fenstergröße für gleitenden Durchschnitt
            
        Returns:
            dict: Trend-Analyseergebnisse
        """
        # Gleitender Durchschnitt
        rolling_mean = self.data[self.value_column].rolling(window=window).mean()
        
        # Mann-Kendall-Trendtest
        values = self.data[self.value_column].dropna().values
        trend_result = stats.kendalltau(range(len(values)), values)
        
        return {
            'rolling_mean': rolling_mean,
            'trend_correlation': trend_result.correlation,
            'trend_pvalue': trend_result.pvalue
        }

# Beispielnutzung:
if __name__ == "__main__":
    # Beispieldaten erstellen
    dates = pd.date_range(start='2023-01-01', end='2023-12-31', freq='D')
    np.random.seed(42)
    values = np.random.normal(100, 10, len(dates))
    values += np.sin(np.arange(len(dates)) * 2 * np.pi / 365) * 20  # Saisonaler Effekt
    values += np.arange(len(dates)) * 0.1  # Trend
    
    # Anomalien hinzufügen
    anomaly_indices = np.random.choice(len(dates), 10, replace=False)
    values[anomaly_indices] += np.random.normal(0, 30, len(anomaly_indices))
    
    df = pd.DataFrame({
        'date': dates,
        'value': values
    })
    
    # Analyzer initialisieren
    analyzer = TimeSeriesAnalyzer(df)
    
    # Zerlegung durchführen
    components = analyzer.decompose_series()
    
    # Anomalien erkennen
    anomalies = analyzer.detect_anomalies()
    
    # Trend analysieren
    trend_analysis = analyzer.analyze_trend()
    
    # Vorhersagen erstellen
    arima_forecast = analyzer.forecast_arima()
    exp_smoothing_forecast = analyzer.forecast_exponential_smoothing(seasonal_periods=365)
    
    # Ergebnisse ausgeben
    print("Erkannte Anomalien:", sum(anomalies))
    print("Trend p-value:", trend_analysis['trend_pvalue'])
    print("ARIMA Vorhersage (erste 5 Werte):", arima_forecast[:5])