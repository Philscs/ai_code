from flask import Flask, request
from flask_jwt_extended import JWTManager, jwt_required, create_access_token
from flask_limiter import Limiter
from flask_limiter.util import get_remote_address
from flask_apispec import FlaskApiSpec, MethodResource, doc
from flask_apispec.annotations import marshal_with, use_kwargs
from flask_apispec.extension import FlaskApiSpec

from argon2 import PasswordHasher

app = Flask(__name__)
app.config['JWT_SECRET_KEY'] = 'your-secret-key'
app.config['JWT_ACCESS_TOKEN_EXPIRES'] = 3600

jwt = JWTManager(app)

limiter = Limiter(app, key_func=get_remote_address)

ph = PasswordHasher()

docs = FlaskApiSpec(app)

class UserResource(MethodResource):
    @doc(description='User login')
    @use_kwargs({'username': str, 'password': str})
    def post(self):
        username = request.json.get('username')
        password = request.json.get('password')

        # Validate username and password
        if not username or not password:
            return {'message': 'Invalid username or password'}, 400

        # Verify password using Argon2
        hashed_password = "<hashed_password_from_database>"
        try:
            ph.verify(hashed_password, password)
        except Exception:
            return {'message': 'Invalid username or password'}, 400

        # Generate JWT access token
        access_token = create_access_token(identity=username)

        return {'access_token': access_token}

docs.register(UserResource)

@app.route('/api/user', methods=['POST'])
@limiter.limit("10/minute")
@marshal_with({'access_token': str})
def login():
    return UserResource().post()

if __name__ == '__main__':
    app.run()
