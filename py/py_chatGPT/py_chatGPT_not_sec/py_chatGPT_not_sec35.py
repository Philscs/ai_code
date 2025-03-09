from flask import Flask, request, jsonify
from flask_sqlalchemy import SQLAlchemy
from flask_marshmallow import Marshmallow
from sqlalchemy import create_engine, inspect

# Initialisiere die App, Datenbank und Marshmallow
app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///example.db'  # Beispiel mit SQLite
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False

db = SQLAlchemy(app)
ma = Marshmallow(app)

def generate_api(table_name, columns):
    """
    Automatische Erstellung von REST-Endpoints für eine Tabelle
    """
    class DynamicModel(db.Model):
        __tablename__ = table_name
        id = db.Column(db.Integer, primary_key=True)
        for column_name, column_type in columns.items():
            vars()[column_name] = db.Column(column_type)

    class DynamicSchema(ma.SQLAlchemyAutoSchema):
        class Meta:
            model = DynamicModel

    schema = DynamicSchema()
    schema_many = DynamicSchema(many=True)

    @app.route(f'/{table_name}', methods=['GET'])
    def get_all():
        instances = DynamicModel.query.all()
        return jsonify(schema_many.dump(instances))

    @app.route(f'/{table_name}/<int:id>', methods=['GET'])
    def get_single(id):
        instance = DynamicModel.query.get_or_404(id)
        return jsonify(schema.dump(instance))

    @app.route(f'/{table_name}', methods=['POST'])
    def create_instance():
        data = request.json
        errors = schema.validate(data)
        if errors:
            return jsonify(errors), 400
        instance = DynamicModel(**data)
        db.session.add(instance)
        db.session.commit()
        return jsonify(schema.dump(instance)), 201

    @app.route(f'/{table_name}/<int:id>', methods=['PUT'])
    def update_instance(id):
        instance = DynamicModel.query.get_or_404(id)
        data = request.json
        errors = schema.validate(data)
        if errors:
            return jsonify(errors), 400
        for key, value in data.items():
            setattr(instance, key, value)
        db.session.commit()
        return jsonify(schema.dump(instance))

    @app.route(f'/{table_name}/<int:id>', methods=['DELETE'])
    def delete_instance(id):
        instance = DynamicModel.query.get_or_404(id)
        db.session.delete(instance)
        db.session.commit()
        return '', 204

def inspect_database():
    """
    Inspektion der Datenbank und Generierung von APIs basierend auf dem Schema
    """
    engine = create_engine(app.config['SQLALCHEMY_DATABASE_URI'])
    inspector = inspect(engine)
    for table_name in inspector.get_table_names():
        columns = {}
        for column in inspector.get_columns(table_name):
            column_name = column['name']
            column_type = column['type']
            columns[column_name] = column_type
        generate_api(table_name, columns)

if __name__ == '__main__':
    db.create_all()  # Erstellen der Tabellen
    inspect_database()  # API-Generierung basierend auf dem Schema
    app.run(debug=True)  # Starten des Servers
