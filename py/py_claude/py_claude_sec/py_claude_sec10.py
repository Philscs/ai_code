from abc import ABC, abstractmethod
import ldap
from typing import Optional, Dict, Any
from datetime import datetime, timedelta
import uuid
import hashlib
import secrets
from dataclasses import dataclass
import jwt
from oauthlib.oauth2 import WebApplicationClient
import requests
from onelogin.saml2.auth import OneLogin_Saml2_Auth
from onelogin.saml2.settings import OneLogin_Saml2_Settings

@dataclass
class UserSession:
    user_id: str
    session_id: str
    auth_method: str
    created_at: datetime
    last_accessed: datetime
    expires_at: datetime
    ip_address: str
    user_agent: str

class AuthenticationResult:
    def __init__(self, success: bool, user_id: Optional[str] = None, error: Optional[str] = None):
        self.success = success
        self.user_id = user_id
        self.error = error

class BaseAuthProvider(ABC):
    @abstractmethod
    def authenticate(self, credentials: Dict[str, Any]) -> AuthenticationResult:
        pass

    @abstractmethod
    def validate_session(self, session_id: str) -> bool:
        pass

class LDAPAuthProvider(BaseAuthProvider):
    def __init__(self, ldap_server: str, base_dn: str):
        self.ldap_server = ldap_server
        self.base_dn = base_dn
        
    def authenticate(self, credentials: Dict[str, Any]) -> AuthenticationResult:
        try:
            ldap_client = ldap.initialize(self.ldap_server)
            ldap_client.protocol_version = ldap.VERSION3
            
            username = credentials.get('username')
            password = credentials.get('password')
            
            if not username or not password:
                return AuthenticationResult(False, error="Missing credentials")
            
            user_dn = f"uid={username},{self.base_dn}"
            ldap_client.simple_bind_s(user_dn, password)
            
            return AuthenticationResult(True, user_id=username)
            
        except ldap.INVALID_CREDENTIALS:
            return AuthenticationResult(False, error="Invalid credentials")
        except Exception as e:
            return AuthenticationResult(False, error=str(e))

class OAuthProvider(BaseAuthProvider):
    def __init__(self, client_id: str, client_secret: str, authorize_url: str, token_url: str):
        self.client = WebApplicationClient(client_id)
        self.client_secret = client_secret
        self.authorize_url = authorize_url
        self.token_url = token_url

    def authenticate(self, credentials: Dict[str, Any]) -> AuthenticationResult:
        try:
            code = credentials.get('code')
            redirect_uri = credentials.get('redirect_uri')
            
            if not code:
                return AuthenticationResult(False, error="Missing authorization code")
                
            token_url, headers, body = self.client.prepare_token_request(
                self.token_url,
                authorization_response=credentials.get('authorization_response'),
                redirect_url=redirect_uri,
                code=code
            )
            
            token_response = requests.post(
                token_url,
                headers=headers,
                data=body,
                auth=(self.client.client_id, self.client_secret),
            )
            
            self.client.parse_request_body_response(token_response.text)
            
            userinfo_response = requests.get(
                credentials.get('userinfo_endpoint'),
                headers={'Authorization': f'Bearer {self.client.token["access_token"]}'},
            )
            
            if userinfo_response.ok:
                userinfo = userinfo_response.json()
                return AuthenticationResult(True, user_id=userinfo.get('sub'))
            
            return AuthenticationResult(False, error="Failed to fetch user info")
            
        except Exception as e:
            return AuthenticationResult(False, error=str(e))

class SAMLProvider(BaseAuthProvider):
    def __init__(self, settings_dict: Dict[str, Any]):
        self.settings = OneLogin_Saml2_Settings(settings_dict)
        
    def authenticate(self, credentials: Dict[str, Any]) -> AuthenticationResult:
        try:
            request_data = credentials.get('request_data')
            if not request_data:
                return AuthenticationResult(False, error="Missing SAML request data")
                
            auth = OneLogin_Saml2_Auth(request_data, self.settings.get_settings_dict())
            
            auth.process_response()
            errors = auth.get_errors()
            
            if errors:
                return AuthenticationResult(False, error=', '.join(errors))
                
            if not auth.is_authenticated():
                return AuthenticationResult(False, error="Authentication failed")
                
            attributes = auth.get_attributes()
            name_id = auth.get_nameid()
            
            return AuthenticationResult(True, user_id=name_id)
            
        except Exception as e:
            return AuthenticationResult(False, error=str(e))

class SessionManager:
    def __init__(self, session_timeout: int = 30):
        self.sessions: Dict[str, UserSession] = {}
        self.session_timeout = session_timeout

    def create_session(self, user_id: str, auth_method: str, ip_address: str, user_agent: str) -> UserSession:
        # Generate a cryptographically secure session ID
        session_id = secrets.token_urlsafe(32)
        
        # Create session with expiration
        now = datetime.utcnow()
        session = UserSession(
            user_id=user_id,
            session_id=session_id,
            auth_method=auth_method,
            created_at=now,
            last_accessed=now,
            expires_at=now + timedelta(minutes=self.session_timeout),
            ip_address=ip_address,
            user_agent=user_agent
        )
        
        self.sessions[session_id] = session
        return session

    def validate_session(self, session_id: str, ip_address: str, user_agent: str) -> bool:
        if session_id not in self.sessions:
            return False
            
        session = self.sessions[session_id]
        now = datetime.utcnow()
        
        # Check for session expiration
        if now > session.expires_at:
            self.destroy_session(session_id)
            return False
            
        # Protect against session hijacking by validating client information
        if session.ip_address != ip_address or session.user_agent != user_agent:
            self.destroy_session(session_id)
            return False
            
        # Update last accessed time and extend expiration
        session.last_accessed = now
        session.expires_at = now + timedelta(minutes=self.session_timeout)
        
        return True

    def destroy_session(self, session_id: str) -> None:
        if session_id in self.sessions:
            del self.sessions[session_id]

    def cleanup_expired_sessions(self) -> None:
        now = datetime.utcnow()
        expired_sessions = [
            session_id for session_id, session in self.sessions.items()
            if now > session.expires_at
        ]
        for session_id in expired_sessions:
            self.destroy_session(session_id)

class AuthenticationManager:
    def __init__(self):
        self.providers: Dict[str, BaseAuthProvider] = {}
        self.session_manager = SessionManager()

    def register_provider(self, name: str, provider: BaseAuthProvider) -> None:
        self.providers[name] = provider

    def authenticate(self, provider_name: str, credentials: Dict[str, Any], 
                    ip_address: str, user_agent: str) -> Optional[UserSession]:
        if provider_name not in self.providers:
            raise ValueError(f"Unknown authentication provider: {provider_name}")

        provider = self.providers[provider_name]
        result = provider.authenticate(credentials)

        if result.success and result.user_id:
            # Create new session
            return self.session_manager.create_session(
                user_id=result.user_id,
                auth_method=provider_name,
                ip_address=ip_address,
                user_agent=user_agent
            )
        return None

    def validate_session(self, session_id: str, ip_address: str, user_agent: str) -> bool:
        return self.session_manager.validate_session(session_id, ip_address, user_agent)

    def logout(self, session_id: str) -> None:
        self.session_manager.destroy_session(session_id)

# Example usage:
if __name__ == "__main__":
    # Initialize authentication manager
    auth_manager = AuthenticationManager()

    # Configure and register LDAP provider
    ldap_provider = LDAPAuthProvider(
        ldap_server="ldap://ldap.example.com",
        base_dn="dc=example,dc=com"
    )
    auth_manager.register_provider("ldap", ldap_provider)

    # Configure and register OAuth provider
    oauth_provider = OAuthProvider(
        client_id="client_id",
        client_secret="client_secret",
        authorize_url="https://oauth.example.com/authorize",
        token_url="https://oauth.example.com/token"
    )
    auth_manager.register_provider("oauth", oauth_provider)

    # Configure and register SAML provider
    saml_settings = {
        "strict": True,
        "debug": True,
        "sp": {
            "entityId": "https://your-app.example.com/metadata/",
            "assertionConsumerService": {
                "url": "https://your-app.example.com/acs/",
                "binding": "urn:oasis:names:tc:SAML:2.0:bindings:HTTP-POST"
            }
        },
        "idp": {
            "entityId": "https://idp.example.com/metadata/",
            "singleSignOnService": {
                "url": "https://idp.example.com/sso/",
                "binding": "urn:oasis:names:tc:SAML:2.0:bindings:HTTP-Redirect"
            },
            "x509cert": "MIIC..."
        }
    }
    saml_provider = SAMLProvider(saml_settings)
    auth_manager.register_provider("saml", saml_provider)

    # Example authentication with LDAP
    sample_credentials = {
        "username": "user1",
        "password": "password123"
    }
    session = auth_manager.authenticate(
        "ldap", 
        sample_credentials,
        ip_address="192.168.1.1",
        user_agent="Mozilla/5.0"
    )

    if session:
        print(f"Authentication successful. Session ID: {session.session_id}")
        
        # Validate session
        is_valid = auth_manager.validate_session(
            session.session_id,
            ip_address="192.168.1.1",
            user_agent="Mozilla/5.0"
        )
        print(f"Session valid: {is_valid}")
        
        # Logout
        auth_manager.logout(session.session_id)