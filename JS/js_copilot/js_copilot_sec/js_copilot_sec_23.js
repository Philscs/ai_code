
const { ApolloServer, PubSub } = require('apollo-server');
const { makeExecutableSchema } = require('@graphql-tools/schema');
const { ApolloServerPluginLandingPageGraphQLPlayground } = require('apollo-server-core');
const { AuthenticationError, ForbiddenError } = require('apollo-server-errors');
const { validate } = require('graphql/validation');
const { rateLimit } = require('graphql-rate-limit');

// Define your GraphQL schema
const typeDefs = `
  type Query {
    hello: String
  }

  type Subscription {
    count: Int
  }
`;

// Define your resolvers
const resolvers = {
  Query: {
    hello: () => 'Hello World',
  },
  Subscription: {
    count: {
      subscribe: () => pubsub.asyncIterator('COUNT_UPDATED'),
    },
  },
};

// Create a PubSub instance for handling subscriptions
const pubsub = new PubSub();

// Create a schema with resolvers
const schema = makeExecutableSchema({ typeDefs, resolvers });

// Apply rate limiting to the schema
const rateLimitDirective = rateLimit({ max: 10, window: '1m' });
schema = rateLimitDirective(schema);

// Apply schema validation
const validateSchema = (requestContext) => {
  const errors = validate(schema, requestContext.document);
  if (errors.length > 0) {
    throw new ForbiddenError('Invalid request');
  }
};
const schemaValidationPlugin = {
  requestDidStart: (requestContext) => {
    validateSchema(requestContext);
    return {
      didResolveOperation: () => {
        validateSchema(requestContext);
      },
    };
  },
};

// Create an Apollo Server instance
const server = new ApolloServer({
  schema,
  plugins: [
    ApolloServerPluginLandingPageGraphQLPlayground(),
    schemaValidationPlugin,
  ],
  context: ({ req }) => {
    // Perform authentication
    const token = req.headers.authorization;
    if (!token) {
      throw new AuthenticationError('Authentication required');
    }
    // Perform resource protection
    if (token !== 'valid_token') {
      throw new ForbiddenError('Access denied');
    }
    return { pubsub };
  },
  subscriptions: {
    path: '/subscriptions',
    onConnect: (connectionParams, webSocket) => {
      // Perform authentication for WebSocket connections
      const token = connectionParams.authorization;
      if (!token) {
        throw new AuthenticationError('Authentication required');
      }
      // Perform resource protection for WebSocket connections
      if (token !== 'valid_token') {
        throw new ForbiddenError('Access denied');
      }
      return { pubsub };
    },
  },
});

// Start the server
server.listen().then(({ url }) => {
  console.log(`Server ready at ${url}`);
});
