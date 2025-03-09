const { GraphQLSchema, Subscription } = require('graphql');
const WebSocket = require('ws');
const rateLimiter = require('rate-limiter-flexible');
const jwt = require('jsonwebtoken');

// Authentifizierungs-Schema
const authSchema = new GraphQLSchema({
    type: 'Query',
    query: `
        type Query {
            authenticate(token: String!): User!
        }
    `,
});

// GraphQL-Subscription-Schema
const subscriptionSchema = new GraphQLSchema({
    type: 'Subscription',
    subscription: `
        type Subscription {
            newMessage(messages: [Message!]!): Message!
        }
    `,
});

// Authentifizierungs-Funktion
async function authenticate(token) {
    try {
        const decodedToken = jwt.verify(token, process.env.SECRET_KEY);
        return decodedToken;
    } catch (error) {
        throw new Error('Invalid token');
    }
}

// WebSocket-Transport
class WebSocketTransport extends Subscription {
    constructor(options) {
        super(options);

        this.ws = new WebSocket.Server({ port: 8080 });

        this.ws.on('connection', (ws) => {
            console.log('Client connected');

            // Verbindung initialisieren
            ws.send(JSON.stringify({
                type: 'INITIALIZE',
                token: options.token,
            }));

            // Abonnement verwalten
            this.abonnement = new Abonnement(ws);
        });
    }
}

// Abonnement-Klasse
class Abonnement {
    constructor(ws) {
        this.ws = ws;
        this.messages = [];
        this.subscriber = null;

        // Abonnement-Einstellungen definieren
        rateLimiter.limit({
            limit: 10, // pro Sekunde
            delay: 1000,
        });

        // Abonnement-Verbindung initialisieren
        ws.on('message', (message) => {
            try {
                const { type, data } = JSON.parse(message);

                if (type === 'NEW_MESSAGE') {
                    this.messages.push(data);
                    this.subscriber?.forEach((subscriber) => subscriber.update(this.messages));
                }
            } catch (error) {
                console.error('Fehler beim Empfangen eines Nachrichtensatzes:', error);
            }
        });
    }

    // Abonnement abschließen
    close() {
        if (this.subscriber) {
            this.subscriber = null;
        }
    }
}

// Anwendungsbereich-Schema
const applicationSchema = new GraphQLSchema({
    type: 'Query',
    query: `
        type Query {
            getMessages: [Message!]!
        }

        type User {
            id: ID!
            token: String!
        }
    `,
});

// Authentifizierungs-Middleware
function authenticateMiddleware(next) {
    return async (req, res, next) => {
        const token = req.headers['authorization'];

        if (!token || !authSchema.authenticate(token)) {
            return res.status(401).json({
                error: 'Unauthorisiert',
            });
        }

        await next();
    };
}

// WebSockets-Route
function websocketRoute(req, res) {
    try {
        const token = req.headers['authorization'];

        // Authentifizierung überprüfen
        authenticateMiddleware((req, res) => {
            const subscriptionOptions = { token };

            // Abonnement konfigurieren
            const subscription = new WebSocketTransport(subscriptionOptions);

            // Ressourcen-Schutz implementieren
            applicationSchema.subscribe(subscription);
        });
    } catch (error) {
        console.error('Fehler beim Starten der Anwendung:', error);
    }
}

// GraphQL-Server starten
const server = new Server(websocketRoute);

server.start();