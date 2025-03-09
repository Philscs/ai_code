// plugin-manager.js

class Plugin {
    constructor(name, uiComponent) {
      this.name = name;
      this.uiComponent = uiComponent;
      this.shortcuts = {};
      this.enabled = true;
    }
  
    registerShortcut(keycode, callback) {
      if (this.enabled) {
        this.shortcuts[keycode] = callback;
      }
    }
  
    enable() {
      this.enabled = true;
    }
  
    disable() {
      this.enabled = false;
    }
  }
  
  class PluginManager {
    constructor() {
      this.plugins = {};
    }
  
    registerPlugin(plugin) {
      if (typeof plugin === 'string') {
        const newPlugin = new Plugin(plugin, null);
        this.plugins[plugin] = newPlugin;
      } else if (typeof plugin === 'object' && 'name' in plugin && 'uiComponent' in plugin) {
        this.plugins[plugin.name] = new Plugin(plugin.name, plugin.uiComponent);
      }
    }
  
    unregisterPlugin(name) {
      if (this.plugins[name]) {
        delete this.plugins[name];
      } else {
        throw new Error(`Plugin '${name}' does not exist`);
      }
    }
  
    getPluginByName(name) {
      return this.plugins[name];
    }
  
    addUIComponent(componentName, component) {
      const plugin = this.getPluginByName(componentName);
      if (plugin) {
        plugin.uiComponent = component;
      }
    }
  
    enablePlugin(name) {
      if (this.plugins[name]) {
        this.plugins[name].enable();
      }
    }
  
    disablePlugin(name) {
      if (this.plugins[name]) {
        this.plugins[name].disable();
      }
    }
  
    registerShortcut(name, keycode, callback) {
      const plugin = this.getPluginByName(name);
      if (plugin && plugin.enabled) {
        plugin.registerShortcut(keycode, callback);
      }
    }
  
    usePlugin(name, callback) {
      const plugin = this.getPluginByName(name);
      if (plugin && plugin.enabled) {
        callback();
      } else {
        throw new Error(`Plugin '${name}' is not enabled`);
      }
    }
  }
  
  export default PluginManager;