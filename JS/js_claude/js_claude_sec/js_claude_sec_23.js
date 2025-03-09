// server.js
const { ApolloServer } = require('@apollo/server');
const { WebSocketServer } = require('ws');
const { useServer } = require('graphql-ws/lib/use/ws');
const jwt = require('jsonwebtoken');
const { RateLimiterMemory } = require('rate-limiter-flexible');
const { makeExecutableSchema } = require('@graphql-tools/schema');

// Schema Definition
const typeDefs = `
  type Subscription {
    messageAdded(channelId: ID!): Message
    userStatusChanged: UserStatus
  }

  type Message {
    id: ID!
    content: String!
    channelId: ID!
    userId: ID!
    timestamp: String!
  }

  type UserStatus {
    userId: ID!
    status: String!
    lastSeen: String!
  }

  type Query {
    # ... other query definitions
  }

  type Mutation {
    # ... other mutation definitions
  }
`;

// Rate Limiter Configuration
const rateLimiter = new RateLimiterMemory({
  points: 10, // Number of connections allowed
  duration: 1, // Per second
});

// Authentication Middleware
const authenticateToken = async (token) => {
  try {
    const decoded = jwt.verify(token, process.env.JWT_SECRET);
    return { userId: decoded.userId, roles: decoded.roles };
  } catch (error) {
    throw new Error('Invalid token');
  }
};

// Resource Protection - Access Control
const resourceGuard = {
  Message: {
    canSubscribe: async (user, channelId) => {
      // Check if user has access to the channel
      const hasAccess = await checkChannelAccess(user.userId, channelId);
      if (!hasAccess) {
        throw new Error('Unauthorized access to channel');
      }
      return true;
    }
  },
  UserStatus: {
    canSubscribe: async (user) => {
      // Check if user has permission to view status updates
      return user.roles.includes('STATUS_VIEWER');
    }
  }
};

// Resolvers
const resolvers = {
  Subscription: {
    messageAdded: {
      subscribe: async (_, { channelId }, context) => {
        // Validate authentication
        if (!context.user) {
          throw new Error('Authentication required');
        }

        // Check resource access
        await resourceGuard.Message.canSubscribe(context.user, channelId);

        // Implementation of subscription logic
        return pubsub.asyncIterator(`MESSAGE_ADDED_${channelId}`);
      }
    },
    userStatusChanged: {
      subscribe: async (_, __, context) => {
        if (!context.user) {
          throw new Error('Authentication required');
        }

        await resourceGuard.UserStatus.canSubscribe(context.user);
        return pubsub.asyncIterator('USER_STATUS_CHANGED');
      }
    }
  }
};

// Create executable schema
const schema = makeExecutableSchema({ typeDefs, resolvers });

// WebSocket Server Setup
const wsServer = new WebSocketServer({
  port: 4000,
  path: '/graphql'
});

// GraphQL WebSocket Server Configuration
useServer({
  schema,
  context: async (ctx) => {
    // Extract token from connection params
    const token = ctx.connectionParams?.authorization;
    if (!token) {
      throw new Error('Authentication required');
    }

    try {
      // Apply rate limiting
      await rateLimiter.consume(ctx.connectionParams?.clientId || 'anonymous');

      // Authenticate user
      const user = await authenticateToken(token);

      return { user };
    } catch (error) {
      if (error.name === 'RateLimiterError') {
        throw new Error('Too many connection attempts. Please try again later.');
      }
      throw error;
    }
  },
  onConnect: async (ctx) => {
    // Additional connection validation
    const { connectionParams } = ctx;
    if (!connectionParams?.clientId) {
      throw new Error('Client ID required');
    }
    return true;
  },
  onDisconnect: async (ctx) => {
    // Cleanup logic
    const user = ctx.context?.user;
    if (user) {
      await updateUserStatus(user.userId, 'offline');
    }
  }
}, wsServer);

// Helper Functions
async function checkChannelAccess(userId, channelId) {
  // Implement channel access verification
  // Return true if user has access, false otherwise
  return true; // Placeholder implementation
}

async function updateUserStatus(userId, status) {
  // Implement user status update logic
  // Publish status change to subscribers
  await pubsub.publish('USER_STATUS_CHANGED', {
    userStatusChanged: {
      userId,
      status,
      lastSeen: new Date().toISOString()
    }
  });
}
