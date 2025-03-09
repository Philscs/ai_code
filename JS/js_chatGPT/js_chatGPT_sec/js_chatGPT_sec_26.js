// Modulsystem
const ModuleSystem = (() => {
    const modules = new Map(); // Registrierte Module
    const loadingModules = new Set(); // Aktuelle Ladevorgänge

    // Abhängigkeitsauflösung
    async function resolveDependencies(dependencies) {
        return Promise.all(
            dependencies.map(async dep => {
                if (!modules.has(dep)) {
                    await loadModule(dep);
                }
                return modules.get(dep);
            })
        );
    }

    // Zirkuläre Referenzen erkennen
    function detectCircularReference(moduleName) {
        if (loadingModules.has(moduleName)) {
            throw new Error(`Circular reference detected for module: ${moduleName}`);
        }
    }

    // Code-Splitting durch dynamischen Import
    async function loadModule(moduleName) {
        detectCircularReference(moduleName);

        loadingModules.add(moduleName);
        try {
            // Dynamischer Import des Moduls
            const module = await import(`./modules/${moduleName}.js`);

            // Sicherheitsüberprüfung
            securityScan(module);

            // Abhängigkeiten auflösen
            const resolvedDependencies = module.dependencies
                ? await resolveDependencies(module.dependencies)
                : [];

            // Modul registrieren
            modules.set(moduleName, { ...module, resolvedDependencies });
        } catch (error) {
            console.error(`Error loading module '${moduleName}':`, error);
        } finally {
            loadingModules.delete(moduleName);
        }
    }

    // Sicherheitsüberprüfung
    function securityScan(module) {
        if (typeof module !== 'object' || !module) {
            throw new Error('Invalid module format');
        }
        if (module.untrustedCode) {
            throw new Error('Security risk: untrusted code detected');
        }
    }

    // Modulladefunktion
    async function require(moduleName) {
        if (!modules.has(moduleName)) {
            await loadModule(moduleName);
        }
        return modules.get(moduleName);
    }

    return {
        require,
    };
})();

// Beispielmodulstruktur:
// ./modules/example.js
// export const dependencies = ['dependency1', 'dependency2'];
// export const someFunction = () => console.log('Hello, World!');

// Nutzung des Modulsystems
(async () => {
    try {
        const exampleModule = await ModuleSystem.require('example');
        exampleModule.someFunction();
    } catch (error) {
        console.error('Failed to load modules:', error);
    }
})();
