from flask import Flask, request, jsonify, session
from flask_cors import CORS
import logging
from flask import Flask, request, jsonify
from werkzeug.security import generate_password_hash, check_password_hash
from flask_session import Session
from flask_cors import CORS

app = Flask(__name__)
app.secret_key = 'your_secret_key'
CORS(app)

# Middleware for request validation
@app.before_request
def validate_request():
    # Validate request for XSS attacks
    if '<script>' in request.data:
        return jsonify({'error': 'XSS attack detected'})

    # Implement CSRF protection
    if request.method == 'POST':
        if 'csrf_token' not in session or session['csrf_token'] != request.form.get('csrf_token'):
            return jsonify({'error': 'CSRF attack detected'})

# Middleware for security headers
@app.after_request
def set_security_headers(response):
    response.headers['X-Content-Type-Options'] = 'nosniff'
    response.headers['X-XSS-Protection'] = '1; mode=block'
    response.headers['Strict-Transport-Security'] = 'max-age=31536000; includeSubDomains'
    return response

# Routes
@app.route('/')
def index():
    return 'Hello, World!'

@app.route('/login', methods=['POST'])
def login():
    # Validate user credentials
    # ...

    # Set session and CSRF token
    session['user_id'] = 1
    session['csrf_token'] = 'your_csrf_token'

    return jsonify({'message': 'Logged in successfully'})

@app.route('/logout', methods=['POST'])
def logout():
    # Clear session
    session.clear()

    return jsonify({'message': 'Logged out successfully'})

if __name__ == '__main__':
    app.run()

    class RequestMiddleware:
        def __init__(self, app):
            self.app = app
            self.logger = logging.getLogger(__name__)

        def __call__(self, environ, start_response):
            # Session-Verwaltung
            session_key = 'session_id'
            if 'SESSION_ID' in environ:
                request.session['session_id'] = environ['SESSION_ID']
            
            # CORS-Konfiguration
            allow_origins = ['*']
            allow_methods = ['GET', 'POST', 'PUT', 'DELETE']
            allow_headers = ['Content-Type', 'Authorization']

            cors_config = {
                'origins': allow_origins,
                'methods': allow_methods,
                'headers': allow_headers
            }

            self.app.config['SESSION_PERMANENT'] = False
            self.app.config['SESSION_TYPE'] = 'filesystem'
            Session(app, session_key=session_key)
            CORS(self.app, cors_config=cors_config)

            # Request-Validierung
            if not request.method in ['GET', 'HEAD']:
                try:
                    data = {}
                    for key, value in request.form.items():
                        data[key] = value
                    for key, value in request.json.items():
                        data[key] = value

                    self.app.validate_request(data)

                    # XSS-Schutz
                    if not self.app.xss_check(request):
                        return 'XSS-Attack', 403

                except Exception as e:
                    return f'Fehler bei der Request-Validierung: {str(e)}', 400

            # Security-Headers
            self.app.set_security_headers()

            response = self.app(self._original_handler(environ, start_response))

            if request.method in ['GET', 'HEAD']:
                return response(200)
            else:
                return response

        def _original_handler(self, environ, start_response):
            raise NotImplementedError('MUST be implemented by subclass')

    class RequestValidator(RequestMiddleware):
        def __init__(self, app):
            super().__init__(app)

        def validate_request(self, data):
            # Hier können Sie Ihre eigene Request-Validierung implementieren
            # Beispielsweise nach bestimmten Regeln oder Pflichtfeldern
            if 'username' in data and not data['username'].strip():
                raise Exception('Benutzername ist erforderlich')
            if 'password' in data and not data['password'].strip():
                raise Exception('Passwort ist erforderlich')

        def xss_check(self, request):
            # Hier können Sie Ihre eigene XSS-Check implementieren
            # Beispielsweise nach bestimmten HTML-Tags oder -Formatierungen
            if '<script>' in str(request.form) or '<style>' in str(request.form):
                return False
            return True

        def set_security_headers(self):
            self.app.add_header('Content-Security-Policy', 'default-src \'self\'')
            self.app.add_header('X-Frame-Options', 'DENY')
            self.app.add_header('X-XSS-Protection', '1; mode=block')

    class CSRFMiddleware(RequestMiddleware):
        def __init__(self, app):
            super().__init__(app)

        def validate_csrf(self, request):
            # Hier können Sie Ihre eigene CSRF-Validierung implementieren
            # Beispielsweise nach einem bestimmten Token oder Formularfeld
            token = request.form.get('csrf_token')
            if not token or token != 'YOUR_CSRF_TOKEN':
                raise Exception('CSRF-Token ist falsch')

    class App:
        def __init__(self):
            self.middleware = []

        def add_middleware(self, middleware):
            self.middleware.append(middleware)

        def validate_request(self, data):
            for middleware in self.middleware:
                if hasattr(middleware, 'validate_request'):
                    try:
                        middleware.validate_request(data)
                    except Exception as e:
                        return f'Fehler bei der Request-Validierung: {str(e)}', 400

        def xss_check(self, request):
            for middleware in self.middleware:
                if hasattr(middleware, 'xss_check'):
                    if not middleware.xss_check(request):
                        return False
            return True

        def set_security_headers(self):
            for middleware in self.middleware:
                if hasattr(middleware, 'set_security_headers'):
                    middleware.set_security_headers()

    # Beispiel für die Verwendung der Middleware
    app = Flask(__name__)
    request_validator = RequestValidator(app)
    csrf_middleware = CSRFMiddleware(app)

    app.add_middleware(request_validator)
    app.add_middleware(csrf_middleware)

    @app.route('/', methods=['GET', 'POST'])
    def index():
        if request.method == 'POST':
            data = {}
            for key, value in request.form.items():
                data[key] = value
            for key, value in request.json.items():
                data[key] = value

            try:
                # Hier können Sie Ihre eigene Request-Validierung implementieren
                app.validate_request(data)
            except Exception as e:
                return f'Fehler bei der Request-Validierung: {str(e)}', 400

            # Hier können Sie Ihre eigene XSS-Check implementieren
            if not app.xss_check(request):
                return 'XSS-Attack', 403

            # Hier können Sie Ihre eigene CSRF-Validierung implementieren
            try:
                csrf_middleware.validate_csrf(request)
            except Exception as e:
                return f'Fehler bei der CSRF-Validierung: {str(e)}', 403

            # Hier werden die Security-Headers gesetzt
            app.set_security_headers()

            return 'Request erfolgreich'

    if __name__ == '__main__':
        app.run(debug=True)
