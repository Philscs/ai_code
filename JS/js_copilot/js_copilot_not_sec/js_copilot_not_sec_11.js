class PluginManager {
  constructor() {
    this.plugins = [];
  }

  registerPlugin(plugin) {
    this.plugins.push(plugin);
  }

  unregisterPlugin(plugin) {
    const index = this.plugins.indexOf(plugin);
    if (index !== -1) {
      this.plugins.splice(index, 1);
    }
  }

  reloadPlugins() {
    this.plugins.forEach(plugin => {
      if (typeof plugin.hotReload === 'function') {
        plugin.hotReload();
      }
    });
  }

  registerUIComponents() {
    this.plugins.forEach(plugin => {
      if (typeof plugin.registerUIComponents === 'function') {
        plugin.registerUIComponents();
      }
    });
  }

  modifyRenderProcess() {
    this.plugins.forEach(plugin => {
      if (typeof plugin.modifyRenderProcess === 'function') {
        plugin.modifyRenderProcess();
      }
    });
  }

  defineKeyboardShortcuts() {
    this.plugins.forEach(plugin => {
      if (typeof plugin.defineKeyboardShortcuts === 'function') {
        plugin.defineKeyboardShortcuts();
      }
    });
  }
}

// Usage example:
const pluginManager = new PluginManager();

// Register plugins
const plugin1 = {
  hotReload() {
    console.log('Plugin 1 hot-reloaded');
  },
  registerUIComponents() {
    console.log('Plugin 1 registered UI components');
  },
  modifyRenderProcess() {
    console.log('Plugin 1 modified render process');
  },
  defineKeyboardShortcuts() {
    console.log('Plugin 1 defined keyboard shortcuts');
  }
};

const plugin2 = {
  hotReload() {
    console.log('Plugin 2 hot-reloaded');
  },
  registerUIComponents() {
    console.log('Plugin 2 registered UI components');
  },
  modifyRenderProcess() {
    console.log('Plugin 2 modified render process');
  },
  defineKeyboardShortcuts() {
    console.log('Plugin 2 defined keyboard shortcuts');
  }
};

pluginManager.registerPlugin(plugin1);
pluginManager.registerPlugin(plugin2);

// Reload plugins
pluginManager.reloadPlugins();

// Register UI components
pluginManager.registerUIComponents();

// Modify render process
pluginManager.modifyRenderProcess();

// Define keyboard shortcuts
pluginManager.defineKeyboardShortcuts();
