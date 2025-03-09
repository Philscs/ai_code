import sqlalchemy as sa
from sqlalchemy import create_engine

# Verbindung zur Datenbank herstellen
engine = create_engine('postgresql://user:password@localhost/dbname')

# Mappings für die Tabellen
Benutzer = sa.Table('Benutzer', engine,
    sa.Column('id', sa.Integer, primary_key=True),
    sa.Column('name', sa.String(255)),
    sa.Column('email', sa.String(255))
)

Rechnung = sa.Table('Rechnung', engine,
    sa.Column('id', sa.Integer, primary_key=True),
    sa.Column('benutzer_id', sa.Integer, sa.ForeignKey('Benutzer.id')),
    sa.Column('datum', sa.Date),
    sa.Column('bruttolink', sa.String(255)),
    sa.Column('nettolink', sa.String(255))
)

# Funktion zur Generierung der REST-API
def generate_rest_api(table):
    # Table-Name als Basis für die API-Namen
    api_name = table.name

    # API-Klasse erstellen
    class ApiClass:
        def __init__(self, db_engine, table):
            self.db_engine = db_engine
            self.table = table

        # HTTP-Methoden implementieren
        def get(self, id=None):
            query = sa.select([table])
            if id:
                query = query.where(table.c.id == id)
            results = engine.execute(query)
            return [dict(row) for row in results]

        def post(self, data):
            # Datenbank-Insert-Query erstellen
            insert_query = sa.insert(table)
            values = {column.name: value for column, value in data.items()}
            query = insert_query.values(values)

            engine.execute(query)
            return 'Neue Rechnung erstellt'

        def put(self, id, data):
            # Datenbank-Update-Query erstellen
            update_query = sa.update(table)
            values = {column.name: value for column, value in data.items()}
            query = update_query.values(values).where(table.c.id == id)

            engine.execute(query)
            return 'Rechnung aktualisiert'

        def delete(self, id):
            # Datenbank-Delete-Query erstellen
            delete_query = sa.delete(table)
            query = delete_query.where(table.c.id == id)

            engine.execute(query)
            return 'Rechnung gelöscht'

    return ApiClass(db_engine, table)

# API für die Benutzer-Tabelle generieren
BenutzerApi = generate_rest_api(Benutzer)

# API für die Rechnungstabelle generieren
RechnungApi = generate_rest_api(Rechnung)