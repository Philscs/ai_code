import pandas as pd
import psycopg2
from sqlalchemy import create_engine
from datetime import datetime
import logging
import hashlib

# Konfiguration der Datenquellen und des Data Warehouses
data_source = {
    'database': 'my_database',
    'username': 'my_username',
    'password': 'my_password'
}

data_warehouse = {
    'host': 'my_host',
    'port': 5439,
    'database': 'my_data_warehouse',
    'username': 'my_user',
    'password': 'my_password'
}

# Funktion für die Daten extraktion
def extract_data(data_source):
    try:
        # Verbindung zur Datenquelle herstellen
        conn = psycopg2.connect(**data_source)
        cur = conn.cursor()

        # SQL-Query zum Abfragen der Daten
        query = """
            SELECT id, name, email FROM customers;
        """

        # Daten extrahieren
        cur.execute(query)
        data = cur.fetchall()

        # Ergebnisse in eine DataFrame umwandeln
        df = pd.DataFrame(data, columns=['id', 'name', 'email'])

        return df

    except Exception as e:
        logging.error(f"Error during extract: {e}")
        raise

# Funktion für die Daten transformation
def transform_data(df):
    try:
        # Daten zu formatieren (z.B. Daten formatieren)
        df['created_at'] = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        df['email_hash'] = df['email'].apply(lambda x: hashlib.sha256(x.encode()).hexdigest())

        return df

    except Exception as e:
        logging.error(f"Error during transform: {e}")
        raise

# Funktion für die Daten load
def load_data(data_warehouse):
    try:
        # Verbindung zum Data Warehouse herstellen
        engine = create_engine('postgresql://{}:{}@{}/{}'.format(
            data_warehouse['username'],
            data_warehouse['password'],
            data_warehouse['host'],
            data_warehouse['database']
        ))

        # Daten in das Data Warehouse loaden
        df.to_sql('customers', con=engine, if_exists='replace', index=False)

    except Exception as e:
        logging.error(f"Error during load: {e}")
        raise

# Hauptfunktion für den ETL-Prozess
def etl_prozess(data_source, data_warehouse):
    try:
        # Daten extrahieren
        df = extract_data(data_source)

        # Daten transformieren
        df = transform_data(df)

        # Daten loaden
        load_data(data_warehouse)

    except Exception as e:
        logging.error(f"Error during ETL process: {e}")
        raise

# Ausführen des ETL-Prozesses
if __name__ == "__main__":
    etl_prozess(data_source, data_warehouse)