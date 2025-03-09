import threading
import queue
import sqlite3
import pandas as pd
import time

# Konfigurationsparameter
AGGREGATION_INTERVAL = 10  # Sekunden
ANOMALY_THRESHOLD = 100  # Beispielhafter Schwellwert für Anomalien
DB_NAME = "sensor_data.db"

# Daten-Persistenz einrichten
def setup_database():
    conn = sqlite3.connect(DB_NAME)
    cursor = conn.cursor()
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS SensorData (
            timestamp TEXT,
            sensor_id TEXT,
            value REAL
        )
    """)
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS Alerts (
            timestamp TEXT,
            sensor_id TEXT,
            value REAL,
            alert_message TEXT
        )
    """)
    conn.commit()
    conn.close()

# Sensordaten in die Datenbank schreiben
def save_to_database(sensor_id, value, alert_message=None):
    conn = sqlite3.connect(DB_NAME)
    cursor = conn.cursor()
    timestamp = pd.Timestamp.now().isoformat()
    cursor.execute(
        "INSERT INTO SensorData (timestamp, sensor_id, value) VALUES (?, ?, ?)",
        (timestamp, sensor_id, value)
    )
    if alert_message:
        cursor.execute(
            "INSERT INTO Alerts (timestamp, sensor_id, value, alert_message) VALUES (?, ?, ?, ?)",
            (timestamp, sensor_id, value, alert_message)
        )
    conn.commit()
    conn.close()

# Anomalieprüfung
def detect_anomaly(sensor_id, value):
    if value > ANOMALY_THRESHOLD:
        alert_message = f"Anomaly detected for sensor {sensor_id}: value {value} exceeds threshold {ANOMALY_THRESHOLD}"
        save_to_database(sensor_id, value, alert_message)
        print(alert_message)
    else:
        save_to_database(sensor_id, value)

# Aggregation der Sensordaten
def aggregate_data():
    while True:
        time.sleep(AGGREGATION_INTERVAL)
        conn = sqlite3.connect(DB_NAME)
        query = "SELECT sensor_id, AVG(value) as avg_value FROM SensorData GROUP BY sensor_id"
        df = pd.read_sql_query(query, conn)
        conn.close()
        print("Aggregated Data:")
        print(df)

# Stream-Processing-Thread
def process_stream(sensor_queue):
    while True:
        try:
            sensor_id, value = sensor_queue.get(timeout=1)  # Verhindert Buffer Overflow
            detect_anomaly(sensor_id, value)
        except queue.Empty:
            continue

# Hauptprogramm
def main():
    setup_database()
    sensor_queue = queue.Queue(maxsize=1000)  # Begrenzte Größe zur Vermeidung von Buffer Overflows

    # Thread für die Datenaggregation starten
    aggregation_thread = threading.Thread(target=aggregate_data, daemon=True)
    aggregation_thread.start()

    # Thread für die Stream-Verarbeitung starten
    stream_thread = threading.Thread(target=process_stream, args=(sensor_queue,), daemon=True)
    stream_thread.start()

    # Beispielhafte Simulation von Sensordaten
    sensor_ids = ["sensor_1", "sensor_2", "sensor_3"]
    while True:
        for sensor_id in sensor_ids:
            value = abs(100 * pd.np.random.randn())  # Beispielhafte Werte
            try:
                sensor_queue.put((sensor_id, value), timeout=1)
            except queue.Full:
                print("Warning: Sensor queue is full. Dropping data.")
        time.sleep(1)

if __name__ == "__main__":
    main()
