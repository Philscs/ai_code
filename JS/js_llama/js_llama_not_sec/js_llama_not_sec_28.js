const fetch = require('node-fetch');
const consoleLog = console.log;

class OpenApiClient {
  constructor(apiUrl) {
    this.apiUrl = apiUrl;
  }

  async getUsersWithLimit(limit) {
    const url = `${this.apiUrl}/users`;
    const params = new URLSearchParams();
    params.append('limit', limit.toString());
    const response = await fetch(url, { method: 'GET', params });
    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }
    return await response.json();
  }

  async getUsersWithoutLimit() {
    const url = `${this.apiUrl}/users`;
    const response = await fetch(url, { method: 'GET' });
    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }
    return await response.json();
  }
}

module.exports = OpenApiClient;
const OpenApiClient = require('./openapi-client');
const client = new OpenApiClient('https://your-api-url.com');

async function main() {
  try {
    const usersWithoutLimit = await client.getUsersWithoutLimit();
    console.log(usersWithoutLimit);

    const usersWithLimit10 = await client.getUsersWithLimit(10);
    console.log(usersWithLimit10);

    // ...
  } catch (error) {
    console.error(error);
  }
}

main();
