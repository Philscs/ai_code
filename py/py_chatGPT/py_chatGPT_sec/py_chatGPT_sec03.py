from flask import Flask, request, jsonify
from flask_sqlalchemy import SQLAlchemy
from flask_jwt_extended import JWTManager, create_access_token, jwt_required
from flask_limiter import Limiter
from flask_limiter.util import get_remote_address
from argon2 import PasswordHasher
from argon2.exceptions import VerifyMismatchError
import re

# Initialisiere Flask App
app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///users.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
app.config['JWT_SECRET_KEY'] = 'super-secret-key'

# Initialisiere Erweiterungen
db = SQLAlchemy(app)
jwt = JWTManager(app)
limiter = Limiter(get_remote_address, app=app)
password_hasher = PasswordHasher()

# Datenbankmodell
class User(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(80), unique=True, nullable=False)
    password = db.Column(db.String(200), nullable=False)

def is_valid_username(username):
    return re.match(r"^[a-zA-Z0-9_]{3,30}$", username) is not None

@app.before_first_request
def create_tables():
    db.create_all()

# Registrierung
@app.route('/register', methods=['POST'])
@limiter.limit("5 per minute")
def register():
    data = request.get_json()
    username = data.get('username')
    password = data.get('password')

    if not username or not password:
        return jsonify({"message": "Username und Passwort sind erforderlich"}), 400

    if not is_valid_username(username):
        return jsonify({"message": "Ungültiger Benutzername."}), 400

    if User.query.filter_by(username=username).first():
        return jsonify({"message": "Benutzername existiert bereits."}), 400

    hashed_password = password_hasher.hash(password)
    new_user = User(username=username, password=hashed_password)
    db.session.add(new_user)
    db.session.commit()

    return jsonify({"message": "Registrierung erfolgreich."}), 201

# Login
@app.route('/login', methods=['POST'])
@limiter.limit("10 per minute")
def login():
    data = request.get_json()
    username = data.get('username')
    password = data.get('password')

    if not username or not password:
        return jsonify({"message": "Username und Passwort sind erforderlich."}), 400

    user = User.query.filter_by(username=username).first()
    if not user:
        return jsonify({"message": "Ungültiger Benutzername oder Passwort."}), 401

    try:
        password_hasher.verify(user.password, password)
    except VerifyMismatchError:
        return jsonify({"message": "Ungültiger Benutzername oder Passwort."}), 401

    access_token = create_access_token(identity=username)
    return jsonify({"access_token": access_token}), 200

# Geschützte Route
@app.route('/protected', methods=['GET'])
@jwt_required()
@limiter.limit("5 per minute")
def protected():
    return jsonify({"message": "Zugriff erlaubt."}), 200

# Automatische API-Dokumentation
@app.route('/')
def api_docs():
    return jsonify({
        "endpoints": {
            "/register": "POST: Registrierung eines neuen Benutzers",
            "/login": "POST: Benutzer-Login",
            "/protected": "GET: Zugriff nur mit JWT-Token"
        },
        "rate_limits": {
            "/register": "Maximal 5 Anfragen pro Minute",
            "/login": "Maximal 10 Anfragen pro Minute",
            "/protected": "Maximal 5 Anfragen pro Minute"
        }
    })

if __name__ == '__main__':
    app.run(debug=True)
