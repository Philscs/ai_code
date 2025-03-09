class OperationalTransform {
    constructor(clientId, documentId) {
      this.clientId = clientId;
      this.documentId = documentId;
      this.lastOpId = 0;
    }
  
    projectAction(action, clientOps) {
      const projectedAction = {};
      Object.keys(action).forEach((key) => {
        const value = action[key];
        if (clientOps.has(key)) {
          projectedAction[key] = value - clientOps.get(key);
        } else {
          projectedAction[key] = value;
        }
      });
      return projectedAction;
    }
  
    update(document) {
      const currentOpId = this.lastOpId;
      this.lastOpId++;
      Object.keys(this).forEach((key) => {
        if (this[key] !== undefined) {
          document[key] += this[key];
        }
      });
    }
  
    resolveConflict(projectedAction, clientOps) {
      return projectedAction;
    }
  }
  
  class ConflictResolution {
    constructor(ot, documentId) {
      this.ot = ot;
      this.documentId = documentId;
      this.currentOpId = 0;
    }
  
    projectAction(action) {
      return this.ot.projectAction(action, this.currentOps);
    }
  
    update() {
      const updatedDocument = {};
      Object.keys(this.currentOps).forEach((key) => {
        if (this.currentOps[key] !== undefined) {
          updatedDocument[key] += this.currentOps[key];
        }
      });
      return updatedDocument;
    }
  
    resolveConflict(projectedAction, clientOps) {
      return this.ot.resolveConflict(projectedAction, clientOps);
    }
  }
  
  class PresenceSystem {
    constructor(ot, clientId) {
      this.ot = ot;
      this.clientId = clientId;
      this.presence = {};
    }
  
    updatePresence(clientId, isPresent) {
      if (this.presence[clientId] !== undefined) {
        delete this.presence[clientId];
      }
      this.presence[clientId] = isPresent;
    }
  
    checkAvailability() {
      return Object.keys(this.presence).length > 0;
    }
  }
  
  class SecurityControls {
    constructor(ot, clientId) {
      this.ot = ot;
      this.clientId = clientId;
      this.securityRules = {};
    }
  
    registerRequest(clientId) {
      if (this.securityRules[clientId] === undefined) {
        this.securityRules[clientId] = true;
      }
    }
  
    checkAvailability(clientId) {
      return this.securityRules[clientId];
    }
  }
  
  class RateLimiting {
    constructor(ot, clientId) {
      this.ot = ot;
      this.clientId = clientId;
      this.requestCount = 0;
      this.maxRequests = 10;
      this.timeWindow = 1000; // Millisekunden
    }
  
    registerRequest() {
      if (this.requestCount < this.maxRequests) {
        this.requestCount++;
        return true;
      } else {
        return false;
      }
    }
  
    checkAvailability() {
      if (this.requestCount < this.maxRequests) {
        return true;
      } else {
        return false;
      }
    }
  }
  
  class RealTimeCollaboration {
    constructor(clientId, documentId) {
      this.ot = new OperationalTransform(clientId, documentId);
      this.cr = new ConflictResolution(this.ot, documentId);
      this.presenceSystem = new PresenceSystem(this.ot, clientId);
      this.securityControls = new SecurityControls(this.ot, clientId);
      this.rateLimiting = new RateLimiting(this.ot, clientId);
    }
  
    updatePresence(clientId, isPresent) {
      this.presenceSystem.updatePresence(clientId, isPresent);
    }
  
    registerRequest() {
      return this.rateLimiting.registerRequest();
    }
  
    checkAvailability() {
      return this.rateLimiting.checkAvailability();
    }
  
    projectAction(action) {
      const projectedAction = this.ot.projectAction(action, { [clientId]: undefined });
      return projectedAction;
    }
  }
  
  const realTimeCollaboration = new RealTimeCollaboration(' clientId ', 'documentId ');
  
  realTimeCollaboration.updatePresence(' client2 ', true);
  console.log(realTimeCollaboration.checkAvailability());
  console.log(realTimeCollaboration.projectAction({ a: 1, b: 2 }));