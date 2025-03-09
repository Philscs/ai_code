const fs = require('fs');
const path = require('path');

class Bundle {
  constructor(entryPoint) {
    this.entryPoints = {};
    this.mapFiles = {};
    this.code = {};

    // Einfacher Entry-Point-Parser
    const content = fs.readFileSync(path.join(__dirname, entryPoint), 'utf8');
    const treeShakeContent = parseTreeShake(content);
    this.parseTreeShake(treeShakeContent);
  }

  parseTreeShake(content) {
    // Einfacher Tree-Shaking-Parser
    return content.split('\n').filter(line => line.trim() !== '');
  }

  addCodeSplit(entryPoint, modulePath) {
    if (!this.code[modulePath]) {
      this.code[modulePath] = '';
    }
    const codeContent = fs.readFileSync(path.join(__dirname, modulePath), 'utf8');
    this.code[modulePath] += codeContent;
  }

  generateSourceMaps() {
    // Einfacher Source-Map-Generator
    return Object.keys(this.mapFiles).map(file => `#${file}\n${this.mapFiles[file]}`);
  }
}

class CodeSplitter {
  constructor(bundle) {
    this.bundle = bundle;
    this.codeSplits = {};
  }

  addCodeSplit(modulePath, entryPoint) {
    if (!this.codeSplits[modulePath]) {
      this.codeSplits[modulePath] = {};
    }
    const codeContent = fs.readFileSync(path.join(__dirname, modulePath), 'utf8');
    this.codeSplits[modulePath][entryPoint] = codeContent;
  }

  generateCode() {
    let output = '';

    for (const module in this.codeSplits) {
      for (const entryPoint in this.codeSplits[module]) {
        const content = this.codeSplits[module][entryPoint];
        output += `export ${content};\n`;
      }
    }

    return output;
  }
}

class HotModuleReplacer {
  constructor(bundle) {
    this.bundle = bundle;
    this.mapFiles = {};
  }

  addMapFile(file, content) {
    this.mapFiles[file] = content;
  }

  replaceModule(modulePath, newContent) {
    if (this.mapFiles[modulePath]) {
      const mapContent = this.mapFiles[modulePath];
      delete this.mapFiles[modulePath];

      // Einfacher Code-Replace-Algorithm
      const content = fs.readFileSync(path.join(__dirname, modulePath), 'utf8');
      const newMapContent = mapContent.replace(new RegExp(content, 'g'), newContent);
      return new Map([['index'], newMapContent]);
    }
  }

  generateOutput() {
    let output = '';

    for (const entryPoint in this.bundle.code) {
      const content = this.bundle.code[entryPoint];
      if (!this.mapFiles[entryPoint]) {
        output += `export ${content};\n`;
      } else {
        output += `import { code } from './${entryPoint}';\n`;
        output += `${code}\n`;
      }
    }

    return output;
  }
}

class MinimalBundle {
  constructor(entryPoints, mapFiles) {
    this.entryPoints = entryPoints.map((entryPoint, index) => ({
      ...this.createEntryPointObject(index),
    }));

    this.codeSplits = {};
    this.hotModuleReplacer = new HotModuleReplacer(this);

    for (const entryPoint of this.entryPoints) {
      if (entryPoint.modulePath) {
        this.addCodeSplit(entryPoint);
      }
    }

    for (const mapFile in mapFiles) {
      this.addMapFile(mapFile, mapFiles[mapFile]);
    }
  }

  createEntryPointObject(index) {
    return {
      name: `index${index}`,
      modulePath: `./${index}.js`,
      code: '',
    };
  }

  addCodeSplit(entryPoint) {
    const content = fs.readFileSync(path.join(__dirname, entryPoint.modulePath), 'utf8');
    this.codeSplits[entryPoint.name] = {
      ...this.entryPoints.find(ep => ep.name === entryPoint.name),
      code: content,
    };
  }

  addMapFile(file, content) {
    this.hotModuleReplacer.addMapFile(file, content);
  }

  generateOutput() {
    const code = new CodeSplitter(this).generateCode();
    const sourceMaps = this.generateSourceMaps();

    return `${code}\n${sourceMaps.join('\n')}`;
  }

  generateSourceMaps() {
    let output = '';
    for (const file in this.hotModuleReplacer.mapFiles) {
      output += `//# sourceMappingURL=${file}.map\n`;
      output += this.hotModuleReplacer.mapFiles[file] + '\n';
    }
    return output;
  }
}

module.exports = MinimalBundle;
