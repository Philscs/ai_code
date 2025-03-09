import logging
from flask import Flask, request, jsonify
from werkzeug.security import generate_password_hash, check_password_hash
from flask_session import Session
from flask_cors import CORS

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
        allow Origins = ['*']
        allow Methods = ['GET', 'POST', 'PUT', 'DELETE']
        allow Headers = ['Content-Type', 'Authorization']

        cors_config = {
            'origins': allow Origins,
            'methods': allow Methods,
            'headers': allow Headers
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
            self.validate_request(data)
        except Exception as e:
            return f'Fehler bei der Request-Validierung: {str(e)}', 400

        # Hier können Sie Ihre eigene XSS-Check implementieren
        if not self.xss_check(request):
            return 'XSS-Attack', 403

        # Hier können Sie Ihre eigene CSRF-Validierung implementieren
        try:
            self.validate_csrf(request)
        except Exception as e:
            return f'Fehler bei der CSRF-Validierung: {str(e)}', 403

        # Hier werden die Security-Headers gesetzt
        self.set_security_headers()

        return 'Request erfolgreich'

if __name__ == '__main__':
    app.run(debug=True)