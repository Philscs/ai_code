import re
import json
from http.cookies import SimpleCookie
from urllib.parse import urlparse
import secrets

class SecurityMiddleware:
    def __init__(self, app, cors_origins=None, session_secret=None):
        self.app = app
        self.cors_origins = cors_origins or []
        self.session_secret = session_secret or secrets.token_hex(16)
        self.sessions = {}

    def __call__(self, environ, start_response):
        # Validate the request and prevent XSS
        self.prevent_xss(environ)

        # Set CORS and security headers
        def custom_start_response(status, headers, exc_info=None):
            self.set_cors_headers(headers, environ)
            self.set_security_headers(headers)
            return start_response(status, headers, exc_info)

        # Manage secure sessions
        environ["session"] = self.get_or_create_session(environ)

        # Validate CSRF token for POST requests
        if environ["REQUEST_METHOD"] == "POST":
            self.validate_csrf_token(environ)

        return self.app(environ, custom_start_response)

    def prevent_xss(self, environ):
        for key, value in environ.items():
            if isinstance(value, str) and re.search(r'<[a-zA-Z/][^>]*>', value):
                raise ValueError("Potential XSS detected")

    def set_cors_headers(self, headers, environ):
        origin = environ.get("HTTP_ORIGIN", "")
        if origin in self.cors_origins:
            headers.append(("Access-Control-Allow-Origin", origin))
            headers.append(("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS"))
            headers.append(("Access-Control-Allow-Headers", "Content-Type, Authorization"))
            headers.append(("Access-Control-Allow-Credentials", "true"))

    def set_security_headers(self, headers):
        headers.append(("X-Content-Type-Options", "nosniff"))
        headers.append(("X-Frame-Options", "DENY"))
        headers.append(("Content-Security-Policy", "default-src 'self'"))
        headers.append(("X-XSS-Protection", "1; mode=block"))
        headers.append(("Strict-Transport-Security", "max-age=31536000; includeSubDomains"))

    def get_or_create_session(self, environ):
        cookie_header = environ.get("HTTP_COOKIE", "")
        cookie = SimpleCookie(cookie_header)

        session_id = cookie.get("session_id")
        if session_id and session_id.value in self.sessions:
            return self.sessions[session_id.value]
        else:
            new_session_id = secrets.token_hex(16)
            self.sessions[new_session_id] = {"csrf_token": secrets.token_hex(16)}
            environ["HTTP_COOKIE"] = f"session_id={new_session_id}; Secure; HttpOnly; SameSite=Strict"
            return self.sessions[new_session_id]

    def validate_csrf_token(self, environ):
        session = environ["session"]
        csrf_token = session.get("csrf_token")

        content_length = int(environ.get("CONTENT_LENGTH", 0))
        body = environ["wsgi.input"].read(content_length).decode("utf-8")
        environ["wsgi.input"].seek(0)
        
        try:
            post_data = json.loads(body)
        except json.JSONDecodeError:
            raise ValueError("Invalid POST data")

        if post_data.get("csrf_token") != csrf_token:
            raise ValueError("Invalid CSRF token")

# Example usage with a WSGI app
if __name__ == "__main__":
    def simple_app(environ, start_response):
        response_body = b"Hello, Secure World!"
        start_response("200 OK", [("Content-Type", "text/plain")])
        return [response_body]

    app = SecurityMiddleware(simple_app, cors_origins=["http://example.com"])

    # Use a WSGI server to serve the app (e.g., gunicorn, uWSGI)
    # e.g., gunicorn: `gunicorn -w 4 'middleware:app'`
