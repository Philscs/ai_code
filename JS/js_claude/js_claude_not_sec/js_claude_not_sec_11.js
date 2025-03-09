// Plugin Manager for Markdown Editor
class PluginManager {
    constructor() {
      this.plugins = new Map();
      this.components = new Map();
      this.shortcuts = new Map();
      
      // Setup hot reload in development
      if (process.env.NODE_ENV === 'development') {
        this.setupHotReload();
      }
      
      // Initialize keyboard shortcut handling
      this.initializeShortcuts();
    }
    
    // Register a new plugin
    registerPlugin(pluginDefinition) {
      const { id, version, components, renderHooks, shortcuts } = pluginDefinition;
      
      if (!id || !version) {
        throw new Error('Plugin must have an id and version');
      }
      
      // Store plugin definition
      this.plugins.set(id, {
        id,
        version,
        renderHooks: renderHooks || {},
        enabled: true
      });
      
      // Register components if provided
      if (components) {
        Object.entries(components).forEach(([name, component]) => {
          this.registerComponent(id, name, component);
        });
      }
      
      // Register shortcuts if provided
      if (shortcuts) {
        shortcuts.forEach(shortcut => {
          this.registerShortcut(id, shortcut);
        });
      }
      
      // Emit plugin registered event
      this.emit('pluginRegistered', { id, version });
    }
    
    // Unregister a plugin
    unregisterPlugin(pluginId) {
      // Remove shortcuts
      const pluginShortcuts = Array.from(this.shortcuts.entries())
        .filter(([_, data]) => data.pluginId === pluginId);
        
      pluginShortcuts.forEach(([key]) => {
        this.shortcuts.delete(key);
      });
      
      // Remove components
      const pluginComponents = Array.from(this.components.entries())
        .filter(([key]) => key.startsWith(`${pluginId}/`));
        
      pluginComponents.forEach(([key]) => {
        this.components.delete(key);
      });
      
      // Remove plugin
      this.plugins.delete(pluginId);
      
      // Emit plugin unregistered event
      this.emit('pluginUnregistered', { id: pluginId });
    }
    
    // Register a UI component
    registerComponent(pluginId, name, component) {
      const key = `${pluginId}/${name}`;
      this.components.set(key, component);
    }
    
    // Get a registered component
    getComponent(pluginId, name) {
      const key = `${pluginId}/${name}`;
      return this.components.get(key);
    }
    
    // Register a keyboard shortcut
    registerShortcut(pluginId, shortcut) {
      const { key, command, handler } = shortcut;
      this.shortcuts.set(key, { pluginId, command, handler });
    }
    
    // Initialize keyboard shortcut handling
    initializeShortcuts() {
      document.addEventListener('keydown', (event) => {
        const key = this.getKeyString(event);
        const shortcut = this.shortcuts.get(key);
        
        if (shortcut && this.plugins.get(shortcut.pluginId)?.enabled) {
          event.preventDefault();
          shortcut.handler(this.getEditorInstance());
        }
      });
    }
    
    // Get standardized key string from keyboard event
    getKeyString(event) {
      const parts = [];
      if (event.ctrlKey) parts.push('Ctrl');
      if (event.altKey) parts.push('Alt');
      if (event.shiftKey) parts.push('Shift');
      if (event.key !== 'Control' && event.key !== 'Alt' && event.key !== 'Shift') {
        parts.push(event.key);
      }
      return parts.join('+');
    }
    
    // Setup hot reload functionality
    setupHotReload() {
      const ws = new WebSocket('ws://localhost:8080');
      
      ws.onmessage = (event) => {
        const { pluginId, code } = JSON.parse(event.data);
        
        try {
          // Unregister existing plugin
          this.unregisterPlugin(pluginId);
          
          // Evaluate and register new plugin code
          const newPlugin = this.evaluatePluginCode(code);
          this.registerPlugin(newPlugin);
          
          console.log(`Hot reloaded plugin: ${pluginId}`);
        } catch (error) {
          console.error(`Failed to hot reload plugin ${pluginId}:`, error);
        }
      };
    }
    
    // Evaluate plugin code in isolated context
    evaluatePluginCode(code) {
      const module = { exports: {} };
      const require = (dep) => {
        // Define allowed dependencies here
        const deps = {
          'marked': window.marked,
          // Add more dependencies as needed
        };
        return deps[dep];
      };
      
      const fn = new Function('module', 'require', code);
      fn(module, require);
      
      return module.exports;
    }
    
    // Apply render hooks to markdown content
    async renderMarkdown(markdown) {
      let content = markdown;
      
      // Apply pre-render hooks
      for (const [pluginId, plugin] of this.plugins) {
        if (plugin.enabled && plugin.renderHooks.beforeRender) {
          content = await plugin.renderHooks.beforeRender(content);
        }
      }
      
      // Perform base markdown rendering
      content = await window.marked(content);
      
      // Apply post-render hooks
      for (const [pluginId, plugin] of this.plugins) {
        if (plugin.enabled && plugin.renderHooks.afterRender) {
          content = await plugin.renderHooks.afterRender(content);
        }
      }
      
      return content;
    }
    
    // Simple event emitter functionality
    emit(event, data) {
      const evt = new CustomEvent(`plugin:${event}`, { detail: data });
      document.dispatchEvent(evt);
    }
    
    // Get editor instance (should be implemented based on your editor)
    getEditorInstance() {
      return window.__EDITOR_INSTANCE;
    }
  }
  
  // Example usage:
  const syntaxHighlighterPlugin = {
    id: 'syntax-highlighter',
    version: '1.0.0',
    
    // Custom UI components
    components: {
      ToolbarButton: () => {
        const button = document.createElement('button');
        button.textContent = 'Toggle Syntax Highlighting';
        button.onclick = () => console.log('Syntax highlighting toggled');
        return button;
      }
    },
    
    // Render hooks
    renderHooks: {
      afterRender: (html) => {
        // Add syntax highlighting to code blocks
        return html.replace(
          /<code>(.*?)<\/code>/g,
          (match, code) => `<code class="highlighted">${code}</code>`
        );
      }
    },
    
    // Keyboard shortcuts
    shortcuts: [{
      key: 'Ctrl+H',
      command: 'toggleHighlighting',
      handler: (editor) => {
        console.log('Toggle highlighting triggered');
      }
    }]
  };
  
  // Initialize and use
  const pluginManager = new PluginManager();
  pluginManager.registerPlugin(syntaxHighlighterPlugin);
  
  export default PluginManager;