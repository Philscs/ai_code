const fs = require('fs');
const path = require('path');

class RouteGenerator {
  constructor(basePath = './api') {
    this.basePath = basePath;
    this.routes = [];
  }

  // Wandelt einen Verzeichnisnamen in einen Route-Parameter um
  parsePathSegment(segment) {
    if (segment.startsWith('[') && segment.endsWith(']')) {
      return ':' + segment.slice(1, -1);
    }
    return segment;
  }

  // Generiert einen Express/Fastify-kompatiblen Handler
  generateHandler(routePath) {
    const params = routePath
      .split('/')
      .filter(segment => segment.startsWith(':'))
      .map(param => param.slice(1));

    return `
async function handle${routePath.replace(/[^a-zA-Z0-9]/g, '')}(req, res) {
  try {
    // Verfügbare Parameter: ${params.join(', ')}
    const params = {
      ${params.map(param => `${param}: req.params.${param}`).join(',\n      ')}
    };
    
    res.json({
      message: 'Route handler for ${routePath}',
      params
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
}`;
  }

  // Durchsucht rekursiv das Verzeichnis und erstellt Route-Definitionen
  scanDirectory(currentPath = this.basePath, parentRoute = '') {
    const items = fs.readdirSync(currentPath, { withFileTypes: true });

    for (const item of items) {
      if (item.isDirectory()) {
        const segment = this.parsePathSegment(item.name);
        const newRoute = path.join(parentRoute, segment);
        
        // Erstelle Route-Handler für das aktuelle Verzeichnis
        this.routes.push({
          path: newRoute,
          handler: this.generateHandler(newRoute)
        });

        // Rekursiv weitere Unterverzeichnisse durchsuchen
        this.scanDirectory(
          path.join(currentPath, item.name),
          newRoute
        );
      }
    }
  }

  // Generiert den kompletten Router-Code
  generateRouterCode(framework = 'express') {
    this.scanDirectory();

    const imports = framework === 'express' 
      ? "const express = require('express');\nconst router = express.Router();"
      : "const fastify = require('fastify')()";

    const routeRegistration = this.routes.map(route => {
      if (framework === 'express') {
        return `router.get('${route.path}', ${route.handler}\n);`;
      } else {
        return `fastify.get('${route.path}', async (request, reply) => {
          ${route.handler.replace('req, res', 'request, reply')}
        });`;
      }
    }).join('\n\n');

    const exports = framework === 'express'
      ? 'module.exports = router;'
      : `
const start = async () => {
  try {
    await fastify.listen({ port: 3000 });
  } catch (err) {
    fastify.log.error(err);
    process.exit(1);
  }
};
start();`;

    return `${imports}\n\n${routeRegistration}\n\n${exports}`;
  }

  // Speichert den generierten Code in einer Datei
  saveToFile(framework = 'express') {
    const code = this.generateRouterCode(framework);
    const filename = framework === 'express' ? 'routes.js' : 'server.js';
    
    fs.writeFileSync(filename, code);
    console.log(`Route handlers generated in ${filename}`);
    
    return code;
  }
}

// Beispiel-Verwendung
const generator = new RouteGenerator('./api');
generator.saveToFile('express'); // oder 'fastify'