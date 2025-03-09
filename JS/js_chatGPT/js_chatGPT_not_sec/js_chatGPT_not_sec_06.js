const { GraphQLObjectType, GraphQLSchema, GraphQLString, GraphQLList, GraphQLNonNull } = require('graphql');

// TypeScript Interfaces als JS-Dummy-Objekte (normalerweise durch Reflection oder Parsing erzeugt)
const interfaces = {
  User: {
    id: 'String',
    posts: 'Post[]',
    profile: 'Profile',
  },
  Post: {
    id: 'String',
    title: 'String',
    content: 'String',
  },
  Profile: {
    age: 'Int',
    bio: 'String',
  },
};

// Utility-Funktion zum Mapping von TypeScript-Typen auf GraphQL-Typen
function mapType(tsType) {
  if (tsType === 'String') return GraphQLString;
  if (tsType === 'Int') return GraphQLInt;
  if (tsType.endsWith('[]')) return new GraphQLList(mapType(tsType.replace('[]', '')));
  return tsType; // Objektname oder Custom-Type (weiteres Mapping nötig)
}

// Funktion zum Erstellen von GraphQL-Typen
function generateGraphQLTypes(interfaces) {
  const typeMap = {};

  Object.keys(interfaces).forEach((typeName) => {
    const fields = {};

    Object.keys(interfaces[typeName]).forEach((fieldName) => {
      const tsType = interfaces[typeName][fieldName];
      fields[fieldName] = { type: mapType(tsType) };
    });

    typeMap[typeName] = new GraphQLObjectType({
      name: typeName,
      fields: () => fields,
    });
  });

  return typeMap;
}

// Generiere die GraphQL-Typen
const types = generateGraphQLTypes(interfaces);

// Beispiel eines RootQuery-Typs
const RootQuery = new GraphQLObjectType({
  name: 'RootQueryType',
  fields: {
    user: {
      type: types.User,
      args: { id: { type: new GraphQLNonNull(GraphQLString) } },
      resolve(parent, args) {
        // Beispiel-Resolver
        return {
          id: args.id,
          posts: [],
          profile: { age: 25, bio: 'Developer' },
        };
      },
    },
  },
});

// GraphQL-Schema erstellen
const schema = new GraphQLSchema({
  query: RootQuery,
});

module.exports = schema;
