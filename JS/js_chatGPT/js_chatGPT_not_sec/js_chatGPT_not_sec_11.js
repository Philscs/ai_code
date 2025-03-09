// PluginManager.js
class PluginManager {
    constructor(editor) {
        this.editor = editor; // Referenz zur Markdown-Editor-Instanz
        this.plugins = new Map(); // Speicher für geladene Plugins
        this.shortcuts = new Map();
    }

    // Plugin laden
    async loadPlugin(pluginPath) {
        if (this.plugins.has(pluginPath)) {
            console.warn(`Plugin ${pluginPath} ist bereits geladen.`);
            return;
        }

        try {
            const { default: plugin } = await import(pluginPath + `?cacheBuster=${Date.now()}`);
            const instance = new plugin(this.editor);

            if (typeof instance.init === 'function') {
                instance.init();
            }

            this.plugins.set(pluginPath, instance);
            console.log(`Plugin ${pluginPath} erfolgreich geladen.`);
        } catch (error) {
            console.error(`Fehler beim Laden des Plugins ${pluginPath}:`, error);
        }
    }

    // Plugin entladen
    unloadPlugin(pluginPath) {
        const plugin = this.plugins.get(pluginPath);

        if (!plugin) {
            console.warn(`Plugin ${pluginPath} ist nicht geladen.`);
            return;
        }

        if (typeof plugin.destroy === 'function') {
            plugin.destroy();
        }

        this.plugins.delete(pluginPath);
        console.log(`Plugin ${pluginPath} erfolgreich entladen.`);
    }

    // Plugin neu laden
    async reloadPlugin(pluginPath) {
        this.unloadPlugin(pluginPath);
        await this.loadPlugin(pluginPath);
    }

    // UI-Komponenten registrieren
    registerUIComponent(component) {
        if (this.editor && typeof this.editor.addUIComponent === 'function') {
            this.editor.addUIComponent(component);
        }
    }

    // In den Render-Prozess eingreifen
    interceptRender(processor) {
        if (this.editor && typeof this.editor.addRenderInterceptor === 'function') {
            this.editor.addRenderInterceptor(processor);
        }
    }

    // Keyboard Shortcuts registrieren
    registerShortcut(shortcut, callback) {
        if (this.shortcuts.has(shortcut)) {
            console.warn(`Shortcut ${shortcut} ist bereits registriert.`);
            return;
        }

        this.shortcuts.set(shortcut, callback);

        // Globale Keydown-Ereignisse überwachen
        document.addEventListener('keydown', (event) => {
            if (event.key === shortcut) {
                event.preventDefault();
                const action = this.shortcuts.get(shortcut);
                if (action) action();
            }
        });
    }
}

// Beispiel für ein Plugin
// MyPlugin.js
export default class MyPlugin {
    constructor(editor) {
        this.editor = editor;
    }

    init() {
        console.log('MyPlugin initialisiert');

        // Beispiel: UI-Komponente hinzufügen
        this.editor.pluginManager.registerUIComponent({
            name: 'MyButton',
            render: () => {
                const button = document.createElement('button');
                button.innerText = 'Klick mich!';
                button.addEventListener('click', () => alert('MyPlugin Button geklickt!'));
                return button;
            }
        });

        // Beispiel: Render-Interceptor registrieren
        this.editor.pluginManager.interceptRender((markdown) => {
            return markdown.replace(/\*\*(.*?)\*\*/g, '<b>$1</b>');
        });

        // Beispiel: Shortcut hinzufügen
        this.editor.pluginManager.registerShortcut('m', () => {
            alert('Shortcut M gedrückt!');
        });
    }

    destroy() {
        console.log('MyPlugin zerstört');
    }
}

// Editor-Instanz erstellen und Plugin-Manager nutzen
const editor = {
    addUIComponent: (component) => {
        console.log('UI-Komponente hinzugefügt:', component.name);
    },
    addRenderInterceptor: (processor) => {
        console.log('Render-Interceptor hinzugefügt');
    }
};

const pluginManager = new PluginManager(editor);
pluginManager.loadPlugin('./MyPlugin.js');
