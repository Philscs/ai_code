// OAuth 2.0 Client mit PKCE und zusätzlichen Sicherheitsfeatures
class OAuth2Client {
    constructor(config) {
      this.config = {
        authorizationEndpoint: config.authorizationEndpoint,
        tokenEndpoint: config.tokenEndpoint,
        clientId: config.clientId,
        redirectUri: config.redirectUri,
        scope: config.scope,
        storage: config.storage || localStorage // Standard: localStorage, kann überschrieben werden
      };
      
      // Sicheres Token-Storage-Interface
      this.tokenStorage = {
        set: (key, value) => {
          const encrypted = this.encrypt(JSON.stringify(value));
          this.config.storage.setItem(key, encrypted);
        },
        get: (key) => {
          const encrypted = this.config.storage.getItem(key);
          if (!encrypted) return null;
          try {
            return JSON.parse(this.decrypt(encrypted));
          } catch (e) {
            console.error('Token Dekodierungsfehler:', e);
            return null;
          }
        },
        remove: (key) => this.config.storage.removeItem(key)
      };
    }
  
    // PKCE: Code Verifier generieren
    async generateCodeVerifier() {
      const array = new Uint8Array(32);
      crypto.getRandomValues(array);
      return base64URLEncode(array);
    }
  
    // PKCE: Code Challenge generieren
    async generateCodeChallenge(codeVerifier) {
      const encoder = new TextEncoder();
      const data = encoder.encode(codeVerifier);
      const hash = await crypto.subtle.digest('SHA-256', data);
      return base64URLEncode(new Uint8Array(hash));
    }
  
    // Sicheren State-Parameter generieren
    generateState() {
      const array = new Uint8Array(16);
      crypto.getRandomValues(array);
      return base64URLEncode(array);
    }
  
    // Authorization Request starten
    async startAuth() {
      const codeVerifier = await this.generateCodeVerifier();
      const codeChallenge = await this.generateCodeChallenge(codeVerifier);
      const state = this.generateState();
  
      // PKCE und State im Storage speichern
      this.tokenStorage.set('pkce_verifier', codeVerifier);
      this.tokenStorage.set('oauth_state', state);
  
      // Authorization URL erstellen
      const authUrl = new URL(this.config.authorizationEndpoint);
      authUrl.searchParams.append('client_id', this.config.clientId);
      authUrl.searchParams.append('redirect_uri', this.config.redirectUri);
      authUrl.searchParams.append('response_type', 'code');
      authUrl.searchParams.append('scope', this.config.scope);
      authUrl.searchParams.append('code_challenge', codeChallenge);
      authUrl.searchParams.append('code_challenge_method', 'S256');
      authUrl.searchParams.append('state', state);
  
      // Zum Authorization Endpoint weiterleiten
      window.location.href = authUrl.toString();
    }
  
    // Authorization Callback verarbeiten
    async handleCallback(callbackUrl) {
      const urlParams = new URLSearchParams(new URL(callbackUrl).search);
      const code = urlParams.get('code');
      const returnedState = urlParams.get('state');
      
      // State-Parameter validieren
      const savedState = this.tokenStorage.get('oauth_state');
      if (returnedState !== savedState) {
        throw new Error('State validation failed');
      }
      
      // Code Verifier abrufen
      const codeVerifier = this.tokenStorage.get('pkce_verifier');
      if (!codeVerifier) {
        throw new Error('Code verifier not found');
      }
  
      // Cleanup
      this.tokenStorage.remove('pkce_verifier');
      this.tokenStorage.remove('oauth_state');
  
      // Token Request durchführen
      return this.requestToken(code, codeVerifier);
    }
  
    // Token Request
    async requestToken(code, codeVerifier, isRefresh = false) {
      const params = new URLSearchParams();
      
      if (isRefresh) {
        params.append('grant_type', 'refresh_token');
        params.append('refresh_token', code);
      } else {
        params.append('grant_type', 'authorization_code');
        params.append('code', code);
        params.append('code_verifier', codeVerifier);
        params.append('redirect_uri', this.config.redirectUri);
      }
      
      params.append('client_id', this.config.clientId);
  
      try {
        const response = await fetch(this.config.tokenEndpoint, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/x-www-form-urlencoded'
          },
          body: params
        });
  
        if (!response.ok) {
          throw new Error('Token request failed');
        }
  
        const tokens = await response.json();
        
        // Token-Rotation: Alte Token löschen und neue speichern
        this.tokenStorage.remove('oauth_tokens');
        this.tokenStorage.set('oauth_tokens', {
          access_token: tokens.access_token,
          refresh_token: tokens.refresh_token,
          expires_at: Date.now() + (tokens.expires_in * 1000)
        });
  
        return tokens;
      } catch (error) {
        console.error('Token request error:', error);
        throw error;
      }
    }
  
    // Token-Management: Automatische Token-Erneuerung
    async getValidAccessToken() {
      const tokens = this.tokenStorage.get('oauth_tokens');
      
      if (!tokens) {
        throw new Error('No tokens found');
      }
  
      // Prüfen ob Token noch gültig ist
      if (Date.now() < tokens.expires_at - 60000) { // 1 Minute Puffer
        return tokens.access_token;
      }
  
      // Token erneuern wenn Refresh Token vorhanden
      if (tokens.refresh_token) {
        try {
          const newTokens = await this.requestToken(tokens.refresh_token, null, true);
          return newTokens.access_token;
        } catch (error) {
          // Bei Fehler: Tokens löschen und neu authentifizieren
          this.tokenStorage.remove('oauth_tokens');
          throw new Error('Token refresh failed, re-authentication required');
        }
      }
  
      throw new Error('No refresh token available');
    }
  
    // Ver- und Entschlüsselung für sicheres Token Storage
    encrypt(data) {
      // Implementierung der Verschlüsselung hier
      // In der Praxis sollte eine sichere Verschlüsselungsmethode verwendet werden
      return btoa(data);
    }
  
    decrypt(data) {
      // Implementierung der Entschlüsselung hier
      // In der Praxis sollte eine sichere Entschlüsselungsmethode verwendet werden
      return atob(data);
    }
  }
  
  // Hilfsfunktion: Base64URL Encoding
  function base64URLEncode(array) {
    return btoa(String.fromCharCode.apply(null, array))
      .replace(/\+/g, '-')
      .replace(/\//g, '_')
      .replace(/=/g, '');
  }
  
  // Beispiel Verwendung:
  const oauthClient = new OAuth2Client({
    authorizationEndpoint: 'https://authorization-server.com/auth',
    tokenEndpoint: 'https://authorization-server.com/token',
    clientId: 'your-client-id',
    redirectUri: 'https://your-app.com/callback',
    scope: 'read write'
  });
  
  // Authorization starten
  async function startLogin() {
    await oauthClient.startAuth();
  }
  
  // Callback verarbeiten
  async function handleOAuthCallback(callbackUrl) {
    try {
      const tokens = await oauthClient.handleCallback(callbackUrl);
      console.log('Authentication successful');
    } catch (error) {
      console.error('Authentication failed:', error);
    }
  }
  
  // Token für API-Requests verwenden
  async function makeAuthenticatedRequest() {
    try {
      const accessToken = await oauthClient.getValidAccessToken();
      // Token in API-Requests verwenden
      const response = await fetch('https://api.example.com/data', {
        headers: {
          'Authorization': `Bearer ${accessToken}`
        }
      });
      // Response verarbeiten
    } catch (error) {
      console.error('API request failed:', error);
    }
  }