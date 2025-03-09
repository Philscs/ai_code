// First we'll create a YAML parser and code generator for the OpenAPI spec
import yaml from 'js-yaml';

class OpenAPIClientGenerator {
  constructor(spec) {
    this.spec = typeof spec === 'string' ? yaml.load(spec) : spec;
    this.baseUrl = this.spec.servers?.[0]?.url || 'http://localhost:3000';
  }

  generateClient() {
    const paths = this.spec.paths || {};
    let clientCode = this.generateClientClass();
    
    // Generate methods for each path
    Object.entries(paths).forEach(([path, pathSpec]) => {
      Object.entries(pathSpec).forEach(([method, operationSpec]) => {
        clientCode += this.generateMethod(path, method, operationSpec);
      });
    });

    clientCode += '}\n';
    return clientCode;
  }

  generateClientClass() {
    return `
class APIClient {
  constructor(config = {}) {
    this.baseUrl = config.baseUrl || '${this.baseUrl}';
    this.headers = {
      'Content-Type': 'application/json',
      ...config.headers
    };
  }

  async request(method, path, options = {}) {
    const url = new URL(path, this.baseUrl);
    
    // Add query parameters
    if (options.query) {
      Object.entries(options.query).forEach(([key, value]) => {
        if (value != null) {
          url.searchParams.append(key, value);
        }
      });
    }

    const response = await fetch(url.toString(), {
      method: method.toUpperCase(),
      headers: this.headers,
      body: options.body ? JSON.stringify(options.body) : undefined,
    });

    if (!response.ok) {
      throw new Error(\`HTTP error! status: \${response.status}\`);
    }

    return response.json();
  }

`;
  }

  generateMethod(path, method, operation) {
    const parameters = operation.parameters || [];
    const queryParams = parameters.filter(p => p.in === 'query');
    const methodName = this.getMethodName(method, path);
    
    // Generate JSDoc for the method
    let code = `
  /**
   * ${operation.summary || `${method.toUpperCase()} ${path}`}
   * ${operation.description || ''}
   *${queryParams.map(p => `
   * @param {${this.getTypeScript(p)}} ${p.name} - ${p.description || ''}`).join('')}
   * @returns {Promise<any>}
   */
`;

    // Generate method signature
    const paramList = queryParams.length 
      ? `{ ${queryParams.map(p => `${p.name}`).join(', ')} } = {}`
      : '';

    code += `  async ${methodName}(${paramList}) {
    return this.request('${method}', '${path}', {
      query: {
        ${queryParams.map(p => `${p.name}`).join(',\n        ')}
      }
    });
  }
`;

    return code;
  }

  getMethodName(method, path) {
    // Convert path to camelCase method name
    const parts = path.split('/').filter(Boolean);
    const name = parts.map((part, index) => {
      // Remove URL parameters
      part = part.replace(/\{([^}]+)\}/g, '$1');
      return index === 0 ? part : part.charAt(0).toUpperCase() + part.slice(1);
    }).join('');
    return `${method}${name.charAt(0).toUpperCase() + name.slice(1)}`;
  }

  getTypeScript(parameter) {
    const type = parameter.type || 'string';
    switch (type) {
      case 'integer':
      case 'number':
        return 'number';
      case 'boolean':
        return 'boolean';
      case 'array':
        return `Array<${this.getTypeScript({ type: parameter.items.type })}>`;
      default:
        return 'string';
    }
  }
}
