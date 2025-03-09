const { resolve } = require('module-resolution');
const ESLint = require('eslint');

// Funktion zur Auflösung von Abhängigkeiten
function getDependencies(moduleName) {
  return resolve([moduleName], []);
}

// Funktion zum Erkennen von Kreislaufschleifen
function hasCircularReference(moduleName, visited = new Set()) {
  const dependencies = getDependencies(moduleName);

  for (const dependency of dependencies) {
    if (!visited.has(dependency)) {
      visited.add(dependency);
      if (hasCircularReference(dependency, visited)) return true;
    }
  }

  return false;
}

// Funktion zum Code-Splitting
function codeSplit(moduleName) {
  const dependencies = getDependencies(moduleName);
  const code = require(moduleName, dependencies);
  return code;
}

// Funktion zur Sicherheitsüberprüfung
function securityScan(code) {
  const lint = new ESLint({
    configFile: '.eslintrc.json',
    rules: {
      'no-unused-vars': 'error',
      // weitere Regeln
    },
  });

  try {
    lint.runSync();
  } catch (err) {
    return false;
  }

  return true;
}

// Funktion zum Laden von Ressourcen
function resourceLoad(moduleName, fileName) {
  const dependencies = getDependencies(moduleName);
  const resource = require(`./${fileName}`, dependencies);
  return resource;
}

// Modulsystem mit allen Funktionen
module.exports = {
  getDependencies,
  hasCircularReference,
  codeSplit,
  securityScan,
  resourceLoad,
};
