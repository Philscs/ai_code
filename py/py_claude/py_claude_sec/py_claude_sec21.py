import secrets
import hmac
import hashlib
from typing import Callable, Dict, List, Optional
from http import cookies
import re
import html
import time

class SecurityMiddleware:
    def __init__(
        self,
        allowed_origins: List[str] = None,
        session_lifetime: int = 3600,
        csrf_token_name: str = 'csrf_token',
        cookie_secret: str = None
    ):
        self.allowed_origins = allowed_origins or []
        self.session_lifetime = session_lifetime
        self.csrf_token_name = csrf_token_name
        self.cookie_secret = cookie_secret or secrets.token_hex(32)
        
    def _generate_csrf_token(self) -> str:
        """Generiert einen sicheren CSRF-Token"""
        return secrets.token_hex(32)
    
    def _validate_csrf_token(self, request_token: str, session_token: str) -> bool:
        """Überprüft, ob der CSRF-Token gültig ist"""
        if not request_token or not session_token:
            return False
        return hmac.compare_digest(request_token, session_token)
    
    def _sanitize_input(self, value: str) -> str:
        """Bereinigt Input von potenziell gefährlichen Zeichen"""
        if isinstance(value, str):
            # HTML-Escaping
            value = html.escape(value)
            # JavaScript-Injection Prevention
            value = re.sub(r'javascript:', '', value, flags=re.IGNORECASE)
            # Weitere XSS-Vektoren entfernen
            value = re.sub(r'data:', '', value, flags=re.IGNORECASE)
            value = re.sub(r'vbscript:', '', value, flags=re.IGNORECASE)
        return value

    def _create_session(self, request: dict) -> str:
        """Erstellt eine neue Session"""
        session_id = secrets.token_urlsafe(32)
        expires = int(time.time()) + self.session_lifetime
        
        session_data = {
            'id': session_id,
            'expires': expires,
            'csrf_token': self._generate_csrf_token()
        }
        
        # Session-Cookie setzen
        cookie = cookies.SimpleCookie()
        cookie['session'] = session_id
        cookie['session']['httponly'] = True
        cookie['session']['secure'] = True
        cookie['session']['samesite'] = 'Strict'
        cookie['session']['path'] = '/'
        
        return session_id, cookie, session_data

    def _get_security_headers(self) -> Dict[str, str]:
        """Generiert Security-Header"""
        return {
            'X-Content-Type-Options': 'nosniff',
            'X-Frame-Options': 'DENY',
            'X-XSS-Protection': '1; mode=block',
            'Strict-Transport-Security': 'max-age=31536000; includeSubDomains',
            'Content-Security-Policy': "default-src 'self'",
            'Referrer-Policy': 'strict-origin-when-cross-origin'
        }

    def process_request(self, request: dict) -> dict:
        """Verarbeitet eingehende Requests"""
        # CORS-Check
        origin = request.get('headers', {}).get('Origin')
        if origin and self.allowed_origins:
            if origin not in self.allowed_origins:
                return {'status': 403, 'body': 'Origin not allowed'}

        # Input-Sanitization für alle Parameter
        if 'params' in request:
            request['params'] = {
                k: self._sanitize_input(v)
                for k, v in request['params'].items()
            }

        # Session-Handling
        session_id = request.get('cookies', {}).get('session')
        if not session_id:
            session_id, cookie, session_data = self._create_session(request)
            request['session'] = session_data
            request['set_cookie'] = cookie
        
        # CSRF-Validierung für nicht-GET Requests
        if request.get('method', '').upper() != 'GET':
            request_token = request.get('headers', {}).get(self.csrf_token_name)
            session_token = request.get('session', {}).get('csrf_token')
            
            if not self._validate_csrf_token(request_token, session_token):
                return {'status': 403, 'body': 'CSRF validation failed'}

        return request

    def process_response(self, response: dict) -> dict:
        """Verarbeitet ausgehende Responses"""
        # Security-Header hinzufügen
        response.setdefault('headers', {})
        response['headers'].update(self._get_security_headers())

        # CORS-Header setzen wenn nötig
        if self.allowed_origins:
            response['headers']['Access-Control-Allow-Origins'] = ', '.join(self.allowed_origins)
            response['headers']['Access-Control-Allow-Methods'] = 'GET, POST, PUT, DELETE, OPTIONS'
            response['headers']['Access-Control-Allow-Headers'] = 'Content-Type, Authorization'
            response['headers']['Access-Control-Max-Age'] = '3600'

        return response

    def __call__(self, get_response: Callable):
        """Middleware-Wrapper für WSGI/ASGI-kompatible Frameworks"""
        def middleware(request, *args, **kwargs):
            # Request verarbeiten
            processed_request = self.process_request(request)
            if isinstance(processed_request, dict) and 'status' in processed_request:
                return processed_request

            # Response vom nächsten Handler holen
            response = get_response(processed_request, *args, **kwargs)

            # Response verarbeiten
            return self.process_response(response)

        return middleware


# Beispielverwendung:
"""
security = SecurityMiddleware(
    allowed_origins=['https://example.com'],
    session_lifetime=3600,
    csrf_token_name='X-CSRF-Token'
)

@security
def application(request):
    # Ihre Anwendungslogik hier
    return {'status': 200, 'body': 'Hello, secure world!'}
"""