import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from statsmodels.tsa.arima.model import ARIMA
from sklearn.ensemble import IsolationForest
from scipy.signal import find_peaks

class TimeSeriesAnalyzer:
    def __init__(self, data, date_column, value_column):
        self.data = data
        self.data[date_column] = pd.to_datetime(self.data[date_column])
        self.data.set_index(date_column, inplace=True)
        self.values = self.data[value_column]

    def plot_series(self):
        plt.figure(figsize=(12, 6))
        plt.plot(self.values, label="Time Series", color="blue")
        plt.title("Time Series")
        plt.xlabel("Time")
        plt.ylabel("Value")
        plt.legend()
        plt.show()

    def detect_trends(self):
        rolling_mean = self.values.rolling(window=12).mean()
        plt.figure(figsize=(12, 6))
        plt.plot(self.values, label="Original", color="blue")
        plt.plot(rolling_mean, label="Trend (Rolling Mean)", color="red")
        plt.title("Trend Detection")
        plt.xlabel("Time")
        plt.ylabel("Value")
        plt.legend()
        plt.show()

        return rolling_mean

    def forecast(self, order=(1, 1, 1), steps=12):
        model = ARIMA(self.values, order=order)
        model_fit = model.fit()
        forecast = model_fit.forecast(steps=steps)
        forecast_index = pd.date_range(start=self.values.index[-1], periods=steps + 1, freq='M')[1:]
        forecast_series = pd.Series(forecast, index=forecast_index)

        plt.figure(figsize=(12, 6))
        plt.plot(self.values, label="Original", color="blue")
        plt.plot(forecast_series, label="Forecast", color="green")
        plt.title("ARIMA Forecast")
        plt.xlabel("Time")
        plt.ylabel("Value")
        plt.legend()
        plt.show()

        return forecast_series

    def detect_anomalies(self):
        model = IsolationForest(contamination=0.05, random_state=42)
        self.data['anomaly_score'] = model.fit_predict(self.values.values.reshape(-1, 1))
        anomalies = self.data[self.data['anomaly_score'] == -1]

        plt.figure(figsize=(12, 6))
        plt.plot(self.values, label="Time Series", color="blue")
        plt.scatter(anomalies.index, anomalies[self.values.name], color="red", label="Anomalies")
        plt.title("Anomaly Detection")
        plt.xlabel("Time")
        plt.ylabel("Value")
        plt.legend()
        plt.show()

        return anomalies

# Beispiel-Daten
np.random.seed(42)
dates = pd.date_range(start="2020-01-01", periods=100, freq='M')
data = pd.DataFrame({
    "date": dates,
    "value": np.sin(np.linspace(0, 20, 100)) + np.random.normal(scale=0.5, size=100)
})

# Initialisierung und Analyse
ts_analyzer = TimeSeriesAnalyzer(data, date_column="date", value_column="value")
ts_analyzer.plot_series()
ts_analyzer.detect_trends()
ts_analyzer.forecast(order=(2, 1, 2), steps=12)
ts_analyzer.detect_anomalies()
