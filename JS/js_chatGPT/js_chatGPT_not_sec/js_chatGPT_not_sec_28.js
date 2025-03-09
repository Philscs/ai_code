const yaml = require('js-yaml');
const fs = require('fs');

// Example OpenAPI Specification as YAML
const openApiSpec = `
paths:
  /users:
    get:
      parameters:
        - name: limit
          in: query
          type: integer
`;

// Function to generate API Client code from OpenAPI/Swagger spec
function generateApiClient(yamlSpec) {
  const apiSpec = yaml.load(yamlSpec);

  let clientCode = "// Auto-generated API Client\n";
  clientCode += "const axios = require('axios');\n\n";
  clientCode += "class ApiClient {\n";
  clientCode += "  constructor(baseURL) {\n";
  clientCode += "    this.httpClient = axios.create({ baseURL });\n";
  clientCode += "  }\n\n";

  // Parse paths and methods
  for (const [path, methods] of Object.entries(apiSpec.paths)) {
    for (const [method, config] of Object.entries(methods)) {
      const functionName = `${method}${path.replace(/\//g, '_')}`.replace(/_/g, '').toLowerCase();

      let params = "";
      let queryParams = "";
      if (config.parameters) {
        params = config.parameters.map((param) => param.name).join(', ');
        queryParams = config.parameters
          .filter((param) => param.in === 'query')
          .map((param) => `${param.name}: ${param.name}`)
          .join(', ');
      }

      clientCode += `  async ${functionName}(${params}) {\n`;
      if (queryParams) {
        clientCode += `    const queryParams = { ${queryParams} };\n`;
        clientCode += `    const response = await this.httpClient.${method}('${path}', { params: queryParams });\n`;
      } else {
        clientCode += `    const response = await this.httpClient.${method}('${path}');\n`;
      }
      clientCode += `    return response.data;\n`;
      clientCode += `  }\n\n`;
    }
  }

  clientCode += "}\n\n";
  clientCode += "module.exports = ApiClient;\n";

  return clientCode;
}

// Generate the client code
const generatedCode = generateApiClient(openApiSpec);

// Write the generated code to a file
fs.writeFileSync('ApiClient.js', generatedCode);

console.log("API Client generated successfully!\nCheck the ApiClient.js file.");
