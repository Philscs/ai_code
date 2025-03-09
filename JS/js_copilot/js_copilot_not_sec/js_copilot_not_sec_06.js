const { buildSchema } = require('graphql');

const typeDefs = `
  type User {
    id: String!
    posts: [Post!]!
    profile: Profile!
  }

  type Post {
    # Define the fields for the Post type
  }

  type Profile {
    # Define the fields for the Profile type
  }

  type Query {
    # Define the queries for your schema
  }

  type Mutation {
    # Define the mutations for your schema
  }
`;

const schema = buildSchema(typeDefs);

console.log(schema);
