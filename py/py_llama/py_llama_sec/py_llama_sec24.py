import numpy as np
from scipy import stats
from sklearn.ensemble import IsolationForest
import pandas as pd
import logging

# Einführung des Loggers
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class StreamProcessor:
    def __init__(self, sensor_data, anomaly_threshold=3):
        """
        Initialisiert den Stream-Processor mit den sensorischen Daten und dem Anomalie-Threshold.

        :param sensor_data: Liste von sensorischen Daten
        :param anomaly_threshold: Schwellenwert für die Anomalienerkennung (Standardwerte = 3)
        """
        self.sensor_data = sensor_data
        self.anomaly_threshold = anomaly_threshold

    def _anomaly_detection(self):
        """
        Durchführt die Anomalie-Erkennung mithilfe von Isolation Forest.

        :return: Liste der Anomalien
        """
        model = IsolationForest(n_estimators=100, contamination=0.01)
        anomalies = model.fit_predict(np.array(self.sensor_data).reshape(-1, 1))
        return np.where(anomalies == -1)[0]

    def _data_aggregation(self):
        """
        Durchführt die Aggregation der Daten.

        :return: Gesammelte Daten
        """
        aggregated_data = []
        for i in range(0, len(self.sensor_data), 10):  # aggregieren every 10th Datpunkt
            aggregated_data.append(np.mean(self.sensor_data[i:i+10]))
        return np.array(aggregated_data)

    def _alert_generation(self, anomalies):
        """
        Durchführt die Erzeugung von Warnungen für die Anomalien.

        :param anomalies: Liste der Anomalien
        :return: Liste der Warnungen
        """
        warnings = []
        for anomaly in anomalies:
            warnings.append(f"Anomaly gefunden bei Datpunkt {anomaly}")
        return warnings

    def process(self):
        """
        Durchführt die Echtzeitverarbeitung des Streams.

        :return: Gesammelte Daten und Listen der Anomalien und Warnungen
        """
        anomalies = self._anomaly_detection()
        aggregated_data = self._data_aggregation()
        warnings = self._alert_generation(anomalies)

        # Sicherhe die Datenpersistenz
        try:
            df = pd.DataFrame(aggregated_data, columns=['Agg_Dat'])
            df.to_csv('sensor_data.csv', index=False)
        except Exception as e:
            logger.error(f"Fehler bei der Datenspeicherung: {e}")

        return aggregated_data, anomalies, warnings

# Beispiel für die Verwendung des Stream-Processors
if __name__ == "__main__":
    sensor_data = [1, 2, 3, 4, 5, np.nan, 7, 8, 9, np.nan]
    processor = StreamProcessor(sensor_data)

    aggregated_data, anomalies, warnings = processor.process()

    logger.info(f"Aggregated Data: {aggregated_data}")
    logger.info(f"Anomalies: {anomalies.tolist()}")
    logger.info(f"Warnungen: {warnings.tolist()}")