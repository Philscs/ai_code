// Importiere notwendige Bibliotheken
const crypto = require('crypto');
const storage = require('./storage.js');

class OAuthClient {
  constructor(clientId, redirectUri, authorizationUrl, tokenUrl, scopes) {
    this.clientId = clientId;
    this.redirectUri = redirectUri;
    this.authorizationUrl = authorizationUrl;
    this.tokenUrl = tokenUrl;
    this.scopes = scopes;

    // Generiere einen Zufallscode (Code)
    this.code = crypto.randomBytes(16).toString('hex');

    // Speichere den Code im LocalStorage
    storage.setLocal('oauth:client:code', this.code);
  }

  async authenticate() {
    try {
      // Erstelle eine Authorization-Request mit dem Client-ID, Redirection-URI und Scope
      const authRequest = {
        client_id: this.clientId,
        redirect_uri: this.redirectUri,
        response_type: 'code',
        scope: this.scopes.join(' '),
        state: crypto.randomBytes(16).toString('hex'),
      };

      // Erstelle den Authorization-Code
      const authorizationUrl = `${this.authorizationUrl}?${Object.keys(authRequest).map(key => 
key + '=' + authRequest[key]).join('&')}`;

      // Öffne eine neue Tab-Spalte und führe einen Link zur Authorization-URL aus
      const tabId = window.open(authorizationUrl, '_blank');

      // Warte auf die Rückmeldung vom Server
      while (true) {
        const response = await 
fetch(`${this.tokenUrl}?code=${this.code}&state=${authRequest.state}`, { method: 'GET' });
        if (response.ok && response.headers.get('Location')) {
          break;
        }
        window.setTimeout(() => {}, 1000);
      }

      // Führe den Authorization-Code zurück und speichere ihn im LocalStorage
      storage.setLocal('oauth:client:code', await response.text());
    } catch (error) {
      console.error(error);
    }
  }

  async token() {
    try {
      const code = storage.getLocal('oauth:client:code');
      const tokenRequest = {
        grant_type: 'authorization_code',
        client_id: this.clientId,
        redirect_uri: this.redirectUri,
        code: code,
        scope: this.scopes.join(' '),
      };

      // Erstelle den Token-Request
      const tokenResponse = await fetch(`${this.tokenUrl}`, { method: 'POST', body: 
JSON.stringify(tokenRequest), headers: { 'Content-Type': 'application/json' } });

      if (tokenResponse.ok) {
        const tokenData = await tokenResponse.json();
        return tokenData.access_token;
      } else {
        throw new Error(`Fehler bei Token-Generierung: ${tokenResponse.status}`);
      }
    } catch (error) {
      console.error(error);
    }
  }

  async refreshToken() {
    try {
      const token = storage.getLocal('oauth:client:access_token');
      if (!token) return null;

      // Generiere einen neuen Zufallscode
      const newCode = crypto.randomBytes(16).toString('hex');

      // Speichere den neuen Code im LocalStorage
      storage.setLocal('oauth:client:refresh_token', token);
      storage.setLocal(`oauth:client:new_code`, newCode);

      return newCode;
    } catch (error) {
      console.error(error);
    }
  }

  async rotateRefreshToken() {
    try {
      const refreshToken = storage.getLocal('oauth:client:refresh_token');
      if (!refreshToken) return null;

      // Generiere einen neuen Zufallscode
      const newCode = crypto.randomBytes(16).toString('hex');

      // Speichere den neuen Code im LocalStorage
      storage.setLocal(`oauth:client:new_code`, newCode);

      await this.refreshToken();

      return newCode;
    } catch (error) {
      console.error(error);
    }
  }

  async authorize() {
    try {
      const code = storage.getLocal('oauth:client:code');
      if (!code) return null;

      // Führe eine Authorization-Response mit dem Client-ID, Redirection-URI und Code
      const authResponse = {
        client_id: this.clientId,
        redirect_uri: this.redirectUri,
        code: code,
        state: crypto.randomBytes(16).toString('hex'),
      };

      return fetch(`${this.tokenUrl}`, { method: 'POST', body: JSON.stringify(authResponse), 
headers: { 'Content-Type': 'application/json' } });
    } catch (error) {
      console.error(error);
    }
  }

  async revoke() {
    try {
      const refreshToken = storage.getLocal('oauth:client:refresh_token');
      if (!refreshToken) return null;

      // Generiere einen neuen Zufallscode
      const newCode = crypto.randomBytes(16).toString('hex');

      // Speichere den neuen Code im LocalStorage
      storage.setLocal(`oauth:client:new_code`, newCode);

      await this.refreshToken();

      // Führe eine Revoke-Request mit dem Client-ID und Refresh-Token
      const revokeResponse = fetch(`${this.tokenUrl}/revoke_token?refresh_token=${refreshToken}`, 
{ method: 'DELETE' });

      return revokeResponse;
    } catch (error) {
      console.error(error);
    }
  }
}

module.exports = OAuthClient;
