import numpy as np
from collections import deque
from datetime import datetime
import sqlite3
import logging
import threading
from typing import Dict, List, Optional, Tuple
import queue

class SensorDataProcessor:
    def __init__(self, window_size: int = 100, alert_threshold: float = 3.0,
                 max_buffer_size: int = 1000, db_path: str = "sensor_data.db"):
        """
        Initialisiert den Stream-Processor für Sensordaten.
        
        Args:
            window_size: Größe des gleitenden Fensters für Anomalieerkennung
            alert_threshold: Schwellwert in Standardabweichungen für Anomalien
            max_buffer_size: Maximale Größe des Datenpuffers
            db_path: Pfad zur SQLite-Datenbank
        """
        # Initialisierung der Datenstrukturen
        self.window_size = window_size
        self.alert_threshold = alert_threshold
        self.data_buffer = deque(maxlen=max_buffer_size)
        self.processing_queue = queue.Queue(maxsize=max_buffer_size)
        
        # Logging Setup
        logging.basicConfig(
            level=logging.INFO,
            format='%(asctime)s - %(levelname)s - %(message)s'
        )
        self.logger = logging.getLogger(__name__)
        
        # Datenbankverbindung einrichten
        self.db_path = db_path
        self.setup_database()
        
        # Statistiken für Anomalieerkennung
        self.moving_mean = 0.0
        self.moving_std = 0.0
        self.processed_count = 0
        
        # Worker Thread starten
        self.running = True
        self.processing_thread = threading.Thread(target=self._process_queue)
        self.processing_thread.daemon = True
        self.processing_thread.start()

    def setup_database(self):
        """Erstellt die notwendigen Datenbanktabellen."""
        try:
            with sqlite3.connect(self.db_path) as conn:
                cursor = conn.cursor()
                # Tabelle für Sensordaten
                cursor.execute('''
                    CREATE TABLE IF NOT EXISTS sensor_data (
                        timestamp DATETIME,
                        sensor_id TEXT,
                        value REAL,
                        is_anomaly BOOLEAN
                    )
                ''')
                # Tabelle für Aggregationen
                cursor.execute('''
                    CREATE TABLE IF NOT EXISTS aggregated_data (
                        timestamp DATETIME,
                        sensor_id TEXT,
                        avg_value REAL,
                        min_value REAL,
                        max_value REAL,
                        anomaly_count INTEGER
                    )
                ''')
                conn.commit()
        except sqlite3.Error as e:
            self.logger.error(f"Datenbankfehler: {e}")
            raise

    def ingest_data(self, sensor_id: str, value: float) -> None:
        """
        Nimmt neue Sensordaten entgegen und fügt sie zur Verarbeitungsqueue hinzu.
        
        Args:
            sensor_id: ID des Sensors
            value: Gemessener Wert
        """
        try:
            data_point = {
                'timestamp': datetime.now(),
                'sensor_id': sensor_id,
                'value': value
            }
            self.processing_queue.put(data_point, block=False)
        except queue.Full:
            self.logger.warning("Verarbeitungsqueue ist voll - Daten werden verworfen")

    def _process_queue(self):
        """Verarbeitet kontinuierlich Daten aus der Queue."""
        while self.running:
            try:
                data_point = self.processing_queue.get(timeout=1.0)
                self._process_data_point(data_point)
                self.processing_queue.task_done()
            except queue.Empty:
                continue
            except Exception as e:
                self.logger.error(f"Fehler bei der Datenverarbeitung: {e}")

    def _process_data_point(self, data_point: Dict) -> None:
        """
        Verarbeitet einen einzelnen Datenpunkt.
        
        Args:
            data_point: Dictionary mit Zeitstempel, Sensor-ID und Wert
        """
        # Anomalieerkennung
        is_anomaly = self._detect_anomaly(data_point['value'])
        
        # Datenpersistenz
        self._store_data_point(data_point, is_anomaly)
        
        # Buffer-Update
        self.data_buffer.append(data_point['value'])
        
        # Statistiken aktualisieren
        self._update_statistics(data_point['value'])
        
        # Alert generieren wenn nötig
        if is_anomaly:
            self._generate_alert(data_point)

    def _detect_anomaly(self, value: float) -> bool:
        """
        Erkennt Anomalien basierend auf der Standardabweichung.
        
        Args:
            value: Zu prüfender Wert
            
        Returns:
            bool: True wenn Anomalie erkannt wurde
        """
        if self.processed_count < self.window_size:
            return False
            
        z_score = abs(value - self.moving_mean) / self.moving_std
        return z_score > self.alert_threshold

    def _update_statistics(self, value: float) -> None:
        """
        Aktualisiert die laufenden Statistiken für die Anomalieerkennung.
        
        Args:
            value: Neuer Wert
        """
        self.processed_count += 1
        if self.processed_count == 1:
            self.moving_mean = value
            self.moving_std = 0.0
        else:
            old_mean = self.moving_mean
            self.moving_mean += (value - old_mean) / self.processed_count
            self.moving_std = np.sqrt(
                (self.moving_std ** 2 * (self.processed_count - 2) +
                 (value - old_mean) * (value - self.moving_mean)) /
                (self.processed_count - 1)
            )

    def _store_data_point(self, data_point: Dict, is_anomaly: bool) -> None:
        """
        Speichert einen Datenpunkt in der Datenbank.
        
        Args:
            data_point: Zu speichernder Datenpunkt
            is_anomaly: Anomalie-Flag
        """
        try:
            with sqlite3.connect(self.db_path) as conn:
                cursor = conn.cursor()
                cursor.execute('''
                    INSERT INTO sensor_data
                    (timestamp, sensor_id, value, is_anomaly)
                    VALUES (?, ?, ?, ?)
                ''', (
                    data_point['timestamp'],
                    data_point['sensor_id'],
                    data_point['value'],
                    is_anomaly
                ))
                conn.commit()
        except sqlite3.Error as e:
            self.logger.error(f"Fehler beim Speichern der Daten: {e}")

    def _generate_alert(self, data_point: Dict) -> None:
        """
        Generiert einen Alert für anomale Werte.
        
        Args:
            data_point: Datenpunkt, der den Alert ausgelöst hat
        """
        alert_msg = (
            f"ALERT: Anomalie erkannt für Sensor {data_point['sensor_id']} "
            f"zum Zeitpunkt {data_point['timestamp']}. "
            f"Wert: {data_point['value']}"
        )
        self.logger.warning(alert_msg)

    def aggregate_data(self, interval_minutes: int = 60) -> List[Dict]:
        """
        Aggregiert die gespeicherten Daten über ein bestimmtes Zeitintervall.
        
        Args:
            interval_minutes: Aggregationsintervall in Minuten
            
        Returns:
            List[Dict]: Liste der aggregierten Daten
        """
        try:
            with sqlite3.connect(self.db_path) as conn:
                cursor = conn.cursor()
                cursor.execute('''
                    SELECT 
                        datetime(strftime('%Y-%m-%d %H:00:00', timestamp)) as hour,
                        sensor_id,
                        AVG(value) as avg_value,
                        MIN(value) as min_value,
                        MAX(value) as max_value,
                        SUM(CASE WHEN is_anomaly THEN 1 ELSE 0 END) as anomaly_count
                    FROM sensor_data
                    GROUP BY hour, sensor_id
                    ORDER BY hour DESC
                ''')
                
                results = []
                for row in cursor.fetchall():
                    results.append({
                        'timestamp': row[0],
                        'sensor_id': row[1],
                        'avg_value': row[2],
                        'min_value': row[3],
                        'max_value': row[4],
                        'anomaly_count': row[5]
                    })
                return results
        except sqlite3.Error as e:
            self.logger.error(f"Fehler bei der Datenaggregation: {e}")
            return []

    def shutdown(self):
        """Beendet den Stream-Processor sauber."""
        self.running = False
        self.processing_thread.join()
        self.logger.info("Stream-Processor wurde beendet")

# Beispielverwendung:
if __name__ == "__main__":
    # Stream-Processor initialisieren
    processor = SensorDataProcessor(
        window_size=100,
        alert_threshold=3.0,
        max_buffer_size=1000
    )
    
    try:
        # Beispieldaten einfügen
        import random
        import time
        
        for i in range(1000):
            # Normale Daten mit gelegentlichen Ausreißern
            if random.random() < 0.05:  # 5% Chance auf Ausreißer
                value = random.gauss(100, 30)  # Ausreißer
            else:
                value = random.gauss(100, 10)  # Normale Daten
                
            processor.ingest_data(f"sensor_001", value)
            time.sleep(0.1)  # Simulation von Echtzeit-Daten
            
            # Alle 100 Datenpunkte Aggregation durchführen
            if i % 100 == 0:
                aggregated = processor.aggregate_data()
                print(f"Aggregierte Daten: {aggregated[:3]}")  # Erste 3 Aggregate
                
    except KeyboardInterrupt:
        print("\nProgramm wird beendet...")
    finally:
        processor.shutdown()