// Operational Transform (OT) System
class Operation {
    constructor(type, position, chars = '') {
        this.type = type; // 'insert' or 'delete'
        this.position = position;
        this.chars = chars;
    }

    static transform(op1, op2) {
        if (op1.position < op2.position) {
            return op1;
        }

        if (op1.type === 'insert') {
            return new Operation(op1.type, op1.position + op2.chars.length, op1.chars);
        }

        return new Operation(op1.type, op1.position - op2.chars.length, op1.chars);
    }
}

// Conflict Resolution System
class ConflictResolver {
    constructor() {
        this.version = 0;
        this.history = [];
        this.pending = new Map();
    }

    addOperation(clientId, operation) {
        if (!this.pending.has(clientId)) {
            this.pending.set(clientId, []);
        }

        const transformedOp = this.transformAgainstHistory(operation);
        this.pending.get(clientId).push(transformedOp);
        this.history.push(transformedOp);
        this.version++;

        return transformedOp;
    }

    transformAgainstHistory(operation) {
        let transformed = operation;

        for (const historyOp of this.history) {
            transformed = Operation.transform(transformed, historyOp);
        }

        return transformed;
    }
}

// Presence System
class PresenceSystem {
    constructor() {
        this.users = new Map();
        this.heartbeatInterval = 30000; // 30 seconds
    }

    updatePresence(userId, data) {
        const timestamp = Date.now();
        this.users.set(userId, { ...data, lastSeen: timestamp });
    }

    getActiveUsers() {
        const now = Date.now();
        const activeUsers = [];

        for (const [userId, data] of this.users.entries()) {
            if (now - data.lastSeen <= this.heartbeatInterval) {
                activeUsers.push({ userId, ...data });
            } else {
                this.users.delete(userId);
            }
        }

        return activeUsers;
    }
}

// Security Controls
class SecurityManager {
    constructor() {
        this.sessions = new Map();
        this.permissions = new Map();
    }

    createSession(userId, documentId) {
        const sessionId = crypto.randomUUID();
        this.sessions.set(sessionId, { userId, documentId });
        return sessionId;
    }

    validateSession(sessionId, documentId) {
        const session = this.sessions.get(sessionId);
        return session && session.documentId === documentId;
    }

    setPermissions(documentId, userId, permissions) {
        if (!this.permissions.has(documentId)) {
            this.permissions.set(documentId, new Map());
        }
        this.permissions.get(documentId).set(userId, permissions);
    }

    checkPermission(sessionId, documentId, action) {
        const session = this.sessions.get(sessionId);
        if (!session) return false;

        const docPermissions = this.permissions.get(documentId);
        if (!docPermissions) return false;

        const userPermissions = docPermissions.get(session.userId);
        return userPermissions && userPermissions.includes(action);
    }
}

// Rate Limiting
class RateLimiter {
    constructor(maxRequests = 100, timeWindow = 60000) {
        this.maxRequests = maxRequests;
        this.timeWindow = timeWindow;
        this.requests = new Map();
    }

    isAllowed(clientId) {
        const now = Date.now();
        const clientRequests = this.requests.get(clientId) || [];

        // Remove old requests
        const validRequests = clientRequests.filter(
            timestamp => now - timestamp < this.timeWindow
        );

        if (validRequests.length >= this.maxRequests) {
            return false;
        }

        validRequests.push(now);
        this.requests.set(clientId, validRequests);
        return true;
    }
}

// Main class for the Collaboration System
class CollaborationSystem {
    constructor() {
        this.conflictResolver = new ConflictResolver();
        this.presenceSystem = new PresenceSystem();
        this.securityManager = new SecurityManager();
        this.rateLimiter = new RateLimiter();
        this.documents = new Map();
    }

    connect(userId, documentId) {
        if (!this.rateLimiter.isAllowed(userId)) {
            throw new Error('Rate limit exceeded');
        }

        const sessionId = this.securityManager.createSession(userId, documentId);
        this.presenceSystem.updatePresence(userId, {
            status: 'online',
            documentId
        });

        if (!this.documents.has(documentId)) {
            this.documents.set(documentId, '');
        }

        return {
            sessionId,
            document: this.documents.get(documentId),
            activeUsers: this.presenceSystem.getActiveUsers()
        };
    }

    applyOperation(sessionId, documentId, operation) {
        if (!this.rateLimiter.isAllowed(sessionId)) {
            throw new Error('Rate limit exceeded');
        }

        if (!this.securityManager.checkPermission(sessionId, documentId, 'edit')) {
            throw new Error('Permission denied');
        }

        const session = this.securityManager.sessions.get(sessionId);
        const transformedOp = this.conflictResolver.addOperation(session.userId, operation);

        let document = this.documents.get(documentId);
        if (transformedOp.type === 'insert') {
            document = document.slice(0, transformedOp.position) +
                transformedOp.chars +
                document.slice(transformedOp.position);
        } else if (transformedOp.type === 'delete') {
            document = document.slice(0, transformedOp.position) +
                document.slice(transformedOp.position + transformedOp.chars.length);
        }

        this.documents.set(documentId, document);
        return { document, operation: transformedOp };
    }
}

// Example usage:
const collaborationSystem = new CollaborationSystem();

// Set user permissions
collaborationSystem.securityManager.setPermissions('doc1', 'user1', ['edit', 'view']);
collaborationSystem.securityManager.setPermissions('doc1', 'user2', ['view']);

// Connect user
const connection1 = collaborationSystem.connect('user1', 'doc1');
console.log('User1 connected:', connection1);

// Apply operation
const insertOperation = new Operation('insert', 0, 'Hello World');
const result = collaborationSystem.applyOperation(
    connection1.sessionId,
    'doc1',
    insertOperation
);
console.log('Operation result:', result);

// Get active users
const activeUsers = collaborationSystem.presenceSystem.getActiveUsers();
console.log('Active users:', activeUsers);
