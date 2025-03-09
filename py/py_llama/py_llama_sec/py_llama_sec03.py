import os
from flask import Flask, request, jsonify
from flask_jwt_extended import JWTManager, jwt_required, create_access_token, get_jwt_identity
from flask_sqlalchemy import SQLAlchemy
from werkzeug.security import generate_password_hash, check_password_hash
from argon2 import PasswordHasher
from pydantic import BaseModel

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///database.db'
app.config['JWT_SECRET_KEY'] = os.environ.get('JWT_SECRET_KEY', 'super-secret-key')

db = SQLAlchemy(app)
jwt = JWTManager(app)

ph = PasswordHasher()

class Benutzer(BaseModel):
    username: str
    password: str

class BenutzerRequest(BaseModel):
    username: str
    password: str

def create_user(username, password):
    hashed_password = ph.hash(password)
    user = Benutzer(username=username, password=hashed_password)
    db.session.add(user)
    db.session.commit()

@app.route('/api/login', methods=['POST'])
def login():
    benutzer_request = BenutzerRequest.parse_obj(request.json)
    user = Benutzer.query.filter_by(username=benutzer_request.username).first()
    if not user:
        return jsonify({'error': 'Benutzer nicht gefunden'}), 404

    if check_password_hash(user.password, benutzer_request.password):
        access_token = create_access_token(identity=user.username)
        return jsonify(access_token=access_token), 200
    else:
        return jsonify({'error': 'Falsche Benutzernamen oder Passwort'}), 401

@app.route('/api/register', methods=['POST'])
def register():
    benutzer_request = BenutzerRequest.parse_obj(request.json)
    user = Benutzer.query.filter_by(username=benutzer_request.username).first()
    if not user:
        create_user(benutzer_request.username, benutzer_request.password)
        return jsonify({'message': 'Benutzer erstellt'}), 200
    else:
        return jsonify({'error': 'Benutzername bereits vergeben'}), 400

@app.route('/api/validate', methods=['POST'])
@jwt_required
def validate():
    user_id = get_jwt_identity()
    return jsonify({'message': f'Benutzer {user_id} ist validiert'}), 200

@app.route('/api/rate_limiting', methods=['GET'])
@jwt_required
def rate_limited():
    user_id = get_jwt_identity()
    if request.headers.get('X-RateLimit-Remaining') <= 0:
        return jsonify({'error': 'Rate Limit erreicht'}), 429
    else:
        return jsonify({'message': f'Remaining Rate Limit: 
{request.headers.get("X-RateLimit-Remaining")}'}), 200

@app.route('/api/docs', methods=['GET'])
def docs():
    import uvicorn
    from fastapi.staticfiles import StaticFiles
    static = StaticFiles(directory='static')
    app = FastAPI()
    app.mount('/static', static, name='static')
    return 'API Documentation'
