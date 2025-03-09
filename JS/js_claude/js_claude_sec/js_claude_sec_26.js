// Module Registry für die Verwaltung aller Module
class ModuleRegistry {
    constructor() {
        this.modules = new Map();
        this.loading = new Set();
        this.securityRules = new Map();
    }

    // Registriert ein neues Modul
    register(name, dependencies = [], code) {
        this.modules.set(name, {
            name,
            dependencies,
            code,
            loaded: false,
            instance: null
        });
    }

    // Fügt Sicherheitsregeln hinzu
    addSecurityRule(pattern, validator) {
        this.securityRules.set(pattern, validator);
    }

    // Überprüft die Sicherheit des Codes
    async validateSecurity(name, code) {
        for (const [pattern, validator] of this.securityRules) {
            if (pattern.test(name)) {
                const isValid = await validator(code);
                if (!isValid) {
                    throw new Error(`Security validation failed for module: ${name}`);
                }
            }
        }
        return true;
    }

    // Erkennt zirkuläre Abhängigkeiten
    detectCircularDependencies(name, chain = new Set()) {
        if (chain.has(name)) {
            throw new Error(`Circular dependency detected: ${Array.from(chain).join(' -> ')} -> ${name}`);
        }

        const module = this.modules.get(name);
        if (!module) return;

        chain.add(name);
        for (const dep of module.dependencies) {
            this.detectCircularDependencies(dep, new Set(chain));
        }
    }

    // Lädt ein Modul und seine Abhängigkeiten
    async load(name) {
        if (this.loading.has(name)) {
            throw new Error(`Module ${name} is already loading`);
        }

        const module = this.modules.get(name);
        if (!module) {
            throw new Error(`Module ${name} not found`);
        }

        if (module.loaded) {
            return module.instance;
        }

        this.detectCircularDependencies(name);
        this.loading.add(name);

        try {
            // Lade zuerst alle Abhängigkeiten
            const dependencies = await Promise.all(
                module.dependencies.map(dep => this.load(dep))
            );

            // Validiere die Sicherheit
            await this.validateSecurity(name, module.code);

            // Code-Splitting: Dynamischer Import des Moduls
            const moduleFactory = new Function(...module.dependencies.map(d => d), module.code);
            module.instance = moduleFactory(...dependencies);
            module.loaded = true;

            return module.instance;
        } finally {
            this.loading.delete(name);
        }
    }
}

// Resource Loader für externe Ressourcen
class ResourceLoader {
    constructor() {
        this.cache = new Map();
    }

    // Lädt eine Resource und speichert sie im Cache
    async load(url, type = 'json') {
        if (this.cache.has(url)) {
            return this.cache.get(url);
        }

        try {
            const response = await fetch(url);
            let data;

            switch (type) {
                case 'json':
                    data = await response.json();
                    break;
                case 'text':
                    data = await response.text();
                    break;
                case 'blob':
                    data = await response.blob();
                    break;
                default:
                    throw new Error(`Unsupported resource type: ${type}`);
            }

            this.cache.set(url, data);
            return data;
        } catch (error) {
            throw new Error(`Failed to load resource ${url}: ${error.message}`);
        }
    }

    // Löscht eine Resource aus dem Cache
    invalidate(url) {
        this.cache.delete(url);
    }
}

// Beispiel für die Verwendung
const registry = new ModuleRegistry();
const resourceLoader = new ResourceLoader();

// Sicherheitsregel hinzufügen
registry.addSecurityRule(/.*/, async (code) => {
    // Beispiel für eine einfache Sicherheitsüberprüfung
    const blacklist = ['eval', 'Function', 'with'];
    return !blacklist.some(term => code.includes(term));
});

// Module registrieren
registry.register('logger', [], `
    return {
        log: (message) => console.log('[Logger]', message)
    };
`);

registry.register('api', ['logger'], `
    return {
        fetch: async (url) => {
            logger.log('Fetching: ' + url);
            return fetch(url);
        }
    };
`);

// Beispiel für die Verwendung
async function main() {
    try {
        const api = await registry.load('api');
        const response = await api.fetch('https://api.example.com/data');
        console.log('API Response:', response);
    } catch (error) {
        console.error('Error:', error);
    }
}