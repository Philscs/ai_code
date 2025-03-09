class OAuthClient {
    constructor({ clientId, authUrl, tokenUrl, redirectUri, scopes }) {
        this.clientId = clientId;
        this.authUrl = authUrl;
        this.tokenUrl = tokenUrl;
        this.redirectUri = redirectUri;
        this.scopes = scopes;
        this.state = this.generateRandomString();
        this.codeVerifier = this.generateRandomString();
    }

    generateRandomString(length = 128) {
        const array = new Uint8Array(length);
        crypto.getRandomValues(array);
        return Array.from(array, byte => ('0' + byte.toString(16)).slice(-2)).join('');
    }

    async generateCodeChallenge(verifier) {
        const encoder = new TextEncoder();
        const data = encoder.encode(verifier);
        const hash = await crypto.subtle.digest('SHA-256', data);
        return btoa(String.fromCharCode(...new Uint8Array(hash)))
            .replace(/=/g, '')
            .replace(/\+/g, '-')
            .replace(/\//g, '_');
    }

    saveToken(tokenData) {
        localStorage.setItem('oauth_tokens', JSON.stringify(tokenData));
    }

    getToken() {
        const tokenData = localStorage.getItem('oauth_tokens');
        return tokenData ? JSON.parse(tokenData) : null;
    }

    clearToken() {
        localStorage.removeItem('oauth_tokens');
    }

    async getAuthUrl() {
        const codeChallenge = await this.generateCodeChallenge(this.codeVerifier);
        const params = new URLSearchParams({
            response_type: 'code',
            client_id: this.clientId,
            redirect_uri: this.redirectUri,
            scope: this.scopes.join(' '),
            state: this.state,
            code_challenge: codeChallenge,
            code_challenge_method: 'S256',
        });
        return `${this.authUrl}?${params.toString()}`;
    }

    validateState(receivedState) {
        return this.state === receivedState;
    }

    async exchangeCodeForTokens(code) {
        const body = new URLSearchParams({
            grant_type: 'authorization_code',
            client_id: this.clientId,
            redirect_uri: this.redirectUri,
            code: code,
            code_verifier: this.codeVerifier,
        });

        const response = await fetch(this.tokenUrl, {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: body.toString(),
        });

        if (!response.ok) throw new Error('Failed to exchange code for tokens');
        const tokenData = await response.json();
        this.saveToken(tokenData);
        return tokenData;
    }

    async refreshAccessToken() {
        const tokenData = this.getToken();
        if (!tokenData || !tokenData.refresh_token) throw new Error('No refresh token available');

        const body = new URLSearchParams({
            grant_type: 'refresh_token',
            client_id: this.clientId,
            refresh_token: tokenData.refresh_token,
        });

        const response = await fetch(this.tokenUrl, {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: body.toString(),
        });

        if (!response.ok) throw new Error('Failed to refresh access token');
        const newTokenData = await response.json();
        this.saveToken(newTokenData);
        return newTokenData;
    }

    async startLogin() {
        const authUrl = await this.getAuthUrl();
        window.location.href = authUrl;
    }

    async handleRedirect() {
        const params = new URLSearchParams(window.location.search);
        const code = params.get('code');
        const receivedState = params.get('state');

        if (!this.validateState(receivedState)) throw new Error('Invalid state parameter');

        const tokenData = await this.exchangeCodeForTokens(code);
        return tokenData;
    }
}

const oauthClient = new OAuthClient({
    clientId: 'YOUR_CLIENT_ID',
    authUrl: 'https://example.com/oauth/authorize',
    tokenUrl: 'https://example.com/oauth/token',
    redirectUri: 'https://your-app.com/callback',
    scopes: ['openid', 'profile', 'email'],
});

document.getElementById('loginButton').addEventListener('click', () => {
    oauthClient.startLogin();
});

(async () => {
    if (window.location.search.includes('code')) {
        try {
            const tokens = await oauthClient.handleRedirect();
            console.log('Tokens:', tokens);
        } catch (error) {
            console.error('OAuth Fehler:', error);
        }
    }
})();
