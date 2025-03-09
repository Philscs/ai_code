const { createServer } = require("http");
const { WebSocketServer } = require("ws");
const { useServer } = require("graphql-ws/lib/use/ws");
const { makeExecutableSchema } = require("@graphql-tools/schema");
const { ApolloServer } = require("@apollo/server");
const { expressMiddleware } = require("@apollo/server/express4");
const express = require("express");
const jwt = require("jsonwebtoken");
const rateLimit = require("express-rate-limit");
const { graphqlHTTP } = require("express-graphql");

// Beispiel-Schema
const typeDefs = `
  type Query {
    hello: String
  }
  type Subscription {
    counter: Int
  }
`;

const resolvers = {
  Query: {
    hello: () => "Hallo, Welt!",
  },
  Subscription: {
    counter: {
      subscribe: async function* () {
        let count = 0;
        while (true) {
          await new Promise((resolve) => setTimeout(resolve, 1000));
          yield { counter: count++ };
        }
      },
    },
  },
};

const schema = makeExecutableSchema({ typeDefs, resolvers });

// Authentifizierungsfunktion
const authenticate = (token) => {
  if (!token) throw new Error("Authentication token fehlt");
  try {
    return jwt.verify(token, "SECRET_KEY");
  } catch (err) {
    throw new Error("Ungültiger Token");
  }
};

// Middleware für Schema-Validierung und Authentifizierung
const contextFunction = ({ req }) => {
  const token = req.headers.authorization || "";
  const user = authenticate(token);
  return { user };
};

// Rate-Limiting mit Express Middleware
const limiter = rateLimit({
  windowMs: 60 * 1000, // 1 Minute
  max: 100, // Maximal 100 Anfragen pro Minute
});

const app = express();
app.use(limiter);

// GraphQL HTTP-Endpunkt für Query und Mutation
app.use(
  "/graphql",
  graphqlHTTP({
    schema,
    graphiql: { headerEditorEnabled: true },
    context: contextFunction,
  })
);

const httpServer = createServer(app);
const wsServer = new WebSocketServer({
  server: httpServer,
  path: "/graphql",
});

useServer(
  {
    schema,
    context: (ctx, msg, args) => {
      const token = ctx.connectionParams?.Authorization;
      const user = authenticate(token);
      return { user };
    },
    onConnect: async (ctx) => {
      if (!ctx.connectionParams?.Authorization) {
        throw new Error("Authentication token fehlt");
      }
    },
    onSubscribe: async (ctx, msg) => {
      // Schema-Validierung
      const errors = validate(schema, parse(msg.query));
      if (errors.length > 0) {
        return errors;
      }
      // Ressourcenschutz (Beispiel: Benutzer*innenrollen überprüfen)
      const { user } = ctx;
      if (!user || user.role !== "admin") {
        throw new Error("Zugriff verweigert");
      }
    },
  },
  wsServer
);

httpServer.listen(4000, () => {
  console.log("Server läuft auf http://localhost:4000/graphql");
});
