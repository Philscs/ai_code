import { gql } from 'graphql-tag';

interface User {
  id: string;
  posts: Post[];
  profile: Profile;
}

interface Post {
  id: string;
  title: string;
  content: string;
}

interface Profile {
  name: string;
  email: string;
}

const userQuery = gql`
  type User {
    id: String!
    posts: [Post!]!
    profile: Profile!
  }

  type Post {
    id: String!
    title: String!
    content: String!
  }

  type Profile {
    name: String!
    email: String!
  }
`;

const createUserMutation = gql`
  type User {
    id: String!
    posts: [Post!]!
    profile: Profile!
  }

  type Post {
    id: String!
    title: String!
    content: String!
  }

  type Profile {
    name: String!
    email: String!
  }
`;

const createPostMutation = gql`
  type User {
    id: String!
    posts: [Post!]!
    profile: Profile!
  }

  type Post {
    id: String!
    title: String!
    content: String!
  }

  type Profile {
    name: String!
    email: String!
  }
`;

const createProfileMutation = gql`
  type User {
    id: String!
    posts: [Post!]!
    profile: Profile!
  }

  type Post {
    id: String!
    title: String!
    content: String!
  }

  type Profile {
    name: String!
    email: String!
  }
`;

const resolvers = {
  Query: {
    getUsers() {
      return [
        {
          id: '1',
          posts: [{ id: '2', title: 'Beispiel-Post', content: 'Dies ist ein Beispiel-Post' }],
          profile: { name: 'John Doe', email: 'john@example.com' },
        },
      ];
    },
    getUser(id: string) {
      return {
        id,
        posts: [],
        profile: { name: 'John Doe', email: 'john@example.com' },
      };
    },
  },
  Mutation: {
    createUser(input: UserInput) {
      // Implementierung des Users erstellen
    },
    createPost(id: string, input: PostInput) {
      // Implementierung des Posts erstellen
    },
    createProfile(name: string, email: string) {
      // Implementierung des Profils erstellen
    },
  },
};

interface UserInput {
  name: string;
  email: string;
}

interface PostInput {
  title: string;
  content: string;
}

export { userQuery, createUserMutation, createPostMutation, createProfileMutation, resolvers };