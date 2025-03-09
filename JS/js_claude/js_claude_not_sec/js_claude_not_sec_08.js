// Importiere benötigte Node.js Module
const fs = require('fs');
const path = require('path');
const acorn = require('acorn');
const { walk } = require('estree-walker');
const { SourceMapGenerator } = require('source-map');
const chokidar = require('chokidar');
const ws = require('ws');

class SimpleBundler {
  constructor(entryPoint, outputDir) {
    this.entryPoint = entryPoint;
    this.outputDir = outputDir;
    this.dependencies = new Map();
    this.chunks = new Map();
    this.sourceMapGenerator = new SourceMapGenerator({ file: 'bundle.js' });
    this.hmrClients = new Set();
  }

  // Analysiere ES Module Imports und baue Abhängigkeitsgraph auf
  async analyzeModule(filePath) {
    const content = await fs.promises.readFile(filePath, 'utf-8');
    const ast = acorn.parse(content, {
      sourceType: 'module',
      ecmaVersion: 2020
    });

    const imports = [];
    walk(ast, {
      enter: (node) => {
        if (node.type === 'ImportDeclaration') {
          imports.push({
            source: node.source.value,
            specifiers: node.specifiers.map(spec => ({
              imported: spec.imported?.name || 'default',
              local: spec.local.name
            }))
          });
        }
      }
    });

    return {
      content,
      imports,
      ast
    };
  }

  // Tree-Shaking: Entferne nicht verwendeten Code
  performTreeShaking(ast, usedExports) {
    let code = '';
    walk(ast, {
      enter: (node) => {
        if (node.type === 'ExportNamedDeclaration') {
          const declaration = node.declaration;
          if (declaration && declaration.type === 'VariableDeclaration') {
            const vars = declaration.declarations.filter(dec => 
              usedExports.has(dec.id.name)
            );
            if (vars.length > 0) {
              code += this.generateCode(vars);
            }
          }
        }
      }
    });
    return code;
  }

  // Code-Splitting: Teile Code in Chunks auf
  async splitCode(modules) {
    const chunks = new Map();
    const dynamicImports = new Set();

    // Finde dynamische Imports
    for (const [file, module] of modules) {
      walk(module.ast, {
        enter: (node) => {
          if (node.type === 'ImportExpression') {
            dynamicImports.add(node.source.value);
          }
        }
      });
    }

    // Erstelle Chunks basierend auf dynamischen Imports
    for (const dynamicImport of dynamicImports) {
      const chunk = {
        modules: new Set(),
        dependencies: new Set()
      };
      
      // Füge Module zum Chunk hinzu
      const addToChunk = (module) => {
        chunk.modules.add(module);
        const deps = this.dependencies.get(module);
        if (deps) {
          deps.forEach(dep => {
            if (!chunk.modules.has(dep)) {
              addToChunk(dep);
            }
          });
        }
      };

      addToChunk(dynamicImport);
      chunks.set(dynamicImport, chunk);
    }

    return chunks;
  }

  // Source Map Generierung
  addToSourceMap(originalCode, generatedCode, filename) {
    const originalLines = originalCode.split('\n');
    const generatedLines = generatedCode.split('\n');

    originalLines.forEach((line, index) => {
      if (line.trim()) {
        this.sourceMapGenerator.addMapping({
          generated: {
            line: index + 1,
            column: 0
          },
          original: {
            line: index + 1,
            column: 0
          },
          source: filename
        });
      }
    });

    this.sourceMapGenerator.setSourceContent(filename, originalCode);
  }

  // Hot Module Replacement Setup
  setupHMR() {
    const wss = new ws.Server({ port: 8080 });
    
    wss.on('connection', (client) => {
      this.hmrClients.add(client);
      
      client.on('close', () => {
        this.hmrClients.delete(client);
      });
    });

    // Überwache Dateiänderungen
    chokidar.watch(path.dirname(this.entryPoint)).on('change', async (changedFile) => {
      if (this.dependencies.has(changedFile)) {
        await this.updateModule(changedFile);
        this.notifyClients(changedFile);
      }
    });
  }

  // Benachrichtige HMR Clients über Änderungen
  notifyClients(changedFile) {
    const message = JSON.stringify({
      type: 'update',
      file: changedFile
    });

    this.hmrClients.forEach(client => {
      if (client.readyState === ws.OPEN) {
        client.send(message);
      }
    });
  }

  // Hauptbundle-Methode
  async bundle() {
    // Analysiere Entry Point und baue Abhängigkeitsgraph auf
    const entryModule = await this.analyzeModule(this.entryPoint);
    this.dependencies.set(this.entryPoint, entryModule);

    // Rekursiv alle Abhängigkeiten analysieren
    for (const imp of entryModule.imports) {
      await this.processImport(imp.source, this.entryPoint);
    }

    // Tree-Shaking durchführen
    const usedExports = this.collectUsedExports();
    const optimizedCode = this.performTreeShaking(entryModule.ast, usedExports);

    // Code-Splitting
    const chunks = await this.splitCode(this.dependencies);

    // Generiere Bundle und Source Maps
    const { code, map } = await this.generateBundle(optimizedCode, chunks);

    // Schreibe Output
    if (!fs.existsSync(this.outputDir)) {
      fs.mkdirSync(this.outputDir);
    }

    fs.writeFileSync(
      path.join(this.outputDir, 'bundle.js'),
      code + `\n//# sourceMappingURL=bundle.js.map`
    );

    fs.writeFileSync(
      path.join(this.outputDir, 'bundle.js.map'),
      JSON.stringify(map)
    );

    // Starte HMR wenn gewünscht
    if (process.env.NODE_ENV === 'development') {
      this.setupHMR();
    }

    return {
      stats: {
        entryPoint: this.entryPoint,
        dependencies: this.dependencies.size,
        chunks: chunks.size,
        bundleSize: code.length
      }
    };
  }
}

// Beispielverwendung
const bundler = new SimpleBundler(
  'src/index.js',
  'dist'
);

bundler.bundle().then(stats => {
  console.log('Bundle erstellt:', stats);
});

module.exports = SimpleBundler;