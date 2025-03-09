// TypeScript Interface Parser and GraphQL Schema Generator

class TypeScriptToGraphQLConverter {
    constructor() {
      this.typeMap = new Map();
      this.primitiveTypes = {
        'string': 'String',
        'number': 'Float',
        'boolean': 'Boolean',
        'null': 'String',
        'undefined': 'String'
      };
    }
  
    parseInterface(interfaceStr) {
      // Remove 'interface' keyword and get name
      const nameMatch = interfaceStr.match(/interface\s+(\w+)/);
      if (!nameMatch) throw new Error('Invalid interface declaration');
      
      const interfaceName = nameMatch[1];
      
      // Extract fields
      const fieldsMatch = interfaceStr.match(/{([^}]*)}/);
      if (!fieldsMatch) throw new Error('No fields found in interface');
      
      const fieldsStr = fieldsMatch[1].trim();
      const fields = {};
      
      // Parse each field
      fieldsStr.split(';').forEach(field => {
        field = field.trim();
        if (!field) return;
        
        const [name, type] = field.split(':').map(s => s.trim());
        if (!name || !type) return;
        
        fields[name] = this.parseType(type);
      });
      
      return { name: interfaceName, fields };
    }
  
    parseType(typeStr) {
      // Handle arrays
      if (typeStr.endsWith('[]')) {
        const baseType = typeStr.slice(0, -2);
        return {
          type: 'array',
          ofType: this.parseType(baseType)
        };
      }
      
      // Handle primitive types
      if (this.primitiveTypes[typeStr]) {
        return {
          type: 'primitive',
          name: this.primitiveTypes[typeStr]
        };
      }
      
      // Handle custom types (interfaces)
      return {
        type: 'reference',
        name: typeStr
      };
    }
  
    generateGraphQLSchema(interfaces) {
      let schema = '';
      
      // Generate types for each interface
      interfaces.forEach(interfaceStr => {
        const parsedInterface = this.parseInterface(interfaceStr);
        schema += this.generateType(parsedInterface);
        schema += '\n\n';
      });
      
      return schema;
    }
  
    generateType({ name, fields }) {
      let typeSchema = `type ${name} {\n`;
      
      // Generate fields
      Object.entries(fields).forEach(([fieldName, fieldType]) => {
        typeSchema += `  ${fieldName}: ${this.generateFieldType(fieldType)}\n`;
      });
      
      typeSchema += '}';
      return typeSchema;
    }
  
    generateFieldType(fieldType) {
      switch (fieldType.type) {
        case 'primitive':
          return fieldType.name;
        case 'array':
          return `[${this.generateFieldType(fieldType.ofType)}]`;
        case 'reference':
          return fieldType.name;
        default:
          throw new Error(`Unknown field type: ${fieldType.type}`);
      }
    }
  }
  
  // Example usage
  const converter = new TypeScriptToGraphQLConverter();
  
  const interfaces = [
    `interface User {
      id: string;
      posts: Post[];
      profile: Profile;
    }`,
    `interface Post {
      id: string;
      title: string;
      content: string;
      author: User;
    }`,
    `interface Profile {
      id: string;
      bio: string;
      user: User;
    }`
  ];
  
  const graphqlSchema = converter.generateGraphQLSchema(interfaces);
  console.log(graphqlSchema);