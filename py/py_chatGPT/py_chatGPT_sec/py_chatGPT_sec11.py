import pandas as pd
import sqlalchemy
from cryptography.fernet import Fernet
import logging

# Einrichtung von Logging
logging.basicConfig(
    filename="etl_process.log",
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s",
)

def extract_data(source_config):
    """Extrahiert Daten aus verschiedenen Quellen."""
    try:
        if source_config['type'] == 'csv':
            data = pd.read_csv(source_config['path'])
        elif source_config['type'] == 'database':
            engine = sqlalchemy.create_engine(source_config['connection_string'])
            data = pd.read_sql(source_config['query'], engine)
        else:
            raise ValueError("Unbekannter Quellentyp")
        logging.info("Daten erfolgreich extrahiert aus %s", source_config['path'] if 'path' in source_config else 'Datenbank')
        return data
    except Exception as e:
        logging.error("Fehler beim Extrahieren der Daten: %s", str(e))
        raise

def validate_data(data):
    """Validiert die Daten, um Konsistenz und Vollständigkeit sicherzustellen."""
    try:
        if data.isnull().values.any():
            raise ValueError("Daten enthalten fehlende Werte.")
        logging.info("Datenvalidierung erfolgreich abgeschlossen.")
    except Exception as e:
        logging.error("Fehler bei der Datenvalidierung: %s", str(e))
        raise

def transform_data(data):
    """Transformiert die Daten in das gewünschte Format."""
    try:
        data['timestamp'] = pd.to_datetime(data['timestamp'])
        data['sensitive_column'] = encrypt_sensitive_data(data['sensitive_column'])
        logging.info("Daten erfolgreich transformiert.")
        return data
    except Exception as e:
        logging.error("Fehler bei der Datentransformation: %s", str(e))
        raise

def encrypt_sensitive_data(column):
    """Verschlüsselt sensible Daten mit einem symmetrischen Schlüssel."""
    key = Fernet.generate_key()
    cipher_suite = Fernet(key)
    encrypted_column = column.apply(lambda x: cipher_suite.encrypt(x.encode()).decode())
    logging.info("Sensible Daten erfolgreich verschlüsselt.")
    return encrypted_column

def load_data(data, target_config):
    """Lädt die transformierten Daten in das Data Warehouse."""
    try:
        engine = sqlalchemy.create_engine(target_config['connection_string'])
        data.to_sql(target_config['table'], engine, if_exists='append', index=False)
        logging.info("Daten erfolgreich in das Ziel-Data-Warehouse geladen.")
    except Exception as e:
        logging.error("Fehler beim Laden der Daten: %s", str(e))
        raise

def main():
    source_config = {
        'type': 'csv',
        'path': 'data/source_data.csv'
    }

    target_config = {
        'connection_string': 'sqlite:///data_warehouse.db',
        'table': 'processed_data'
    }

    try:
        # ETL-Prozess
        raw_data = extract_data(source_config)
        validate_data(raw_data)
        transformed_data = transform_data(raw_data)
        load_data(transformed_data, target_config)
        logging.info("ETL-Prozess erfolgreich abgeschlossen.")
    except Exception as e:
        logging.error("ETL-Prozess fehlgeschlagen: %s", str(e))

if __name__ == "__main__":
    main()
