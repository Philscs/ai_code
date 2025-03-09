import queue
import threading
import time
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
        self.data_queue = queue.Queue()
        self.alert_queue = queue.Queue()
        self.running = False

    def start(self):
        self.running = True
        processing_thread = threading.Thread(target=self.process_data)
        processing_thread.start()

    def stop(self):
        self.running = False

    def process_data(self):
        while self.running:
            try:
                data = self.data_queue.get(timeout=1)
                # Perform data processing and anomaly detection
                aggregated_data = self.aggregate_data(data)
                if self.detect_anomaly(aggregated_data):
                    self.alert_queue.put(aggregated_data)
            except queue.Empty:
                continue

    def aggregate_data(self, data):
        """
        Durchführt die Aggregation der Daten.

        :return: Gesammelte Daten
        """
        aggregated_data = []
        for i in range(0, len(data), 10):  # aggregieren every 10th Datpunkt
            aggregated_data.append(np.mean(data[i:i+10]))
        return np.array(aggregated_data)

    def detect_anomaly(self, data):
        """
        Durchführt die Anomalie-Erkennung mithilfe von Isolation Forest.

        :return: True, wenn Anomalie gefunden wurde, sonst False
        """
        model = IsolationForest(n_estimators=100, contamination=0.01)
        anomalies = model.fit_predict(np.array(data).reshape(-1, 1))
        return np.where(anomalies == -1)[0]

    def persist_data(self, data):
        """
        Perform secure data persistence logic here
        """
        try:
            df = pd.DataFrame(data, columns=['Agg_Dat'])
            df.to_csv('sensor_data.csv', index=False)
        except Exception as e:
            logger.error(f"Fehler bei der Datenspeicherung: {e}")

    def generate_alert(self, anomalies):
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
        anomalies = self.detect_anomaly(self.sensor_data)
        aggregated_data = self.aggregate_data(self.sensor_data)
        warnings = self.generate_alert(anomalies)

        self.persist_data(aggregated_data)

        return aggregated_data, anomalies, warnings

# Example usage
if __name__ == "__main__":
    sensor_data = [1, 2, 3, 4, 5, np.nan, 7, 8, 9, np.nan]
    stream_processor = StreamProcessor(sensor_data)
    stream_processor.start()

    # Simulate receiving sensor data
    while True:
        data = receive_sensor_data()
        stream_processor.data_queue.put(data)

    stream_processor.stop()
