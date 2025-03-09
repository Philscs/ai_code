import importlib.util
import os
import sys

class PluginManager:
    def __init__(self, plugins_dir):
        self.plugins_dir = plugins_dir
        self.plugins = {}

    def load_plugin(self, plugin_name):
        if plugin_name in self.plugins:
            return self.plugins[plugin_name]

        plugin_path = os.path.join(self.plugins_dir, plugin_name)
        if not os.path.exists(plugin_path):
            raise ValueError(f"Plugin '{plugin_name}' not found")

        spec = importlib.util.spec_from_file_location(plugin_name, plugin_path)
        plugin_module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(plugin_module)

        self.plugins[plugin_name] = plugin_module
        return plugin_module

    def resolve_dependencies(self, plugin_name):
        plugin_module = self.load_plugin(plugin_name)

        if hasattr(plugin_module, 'dependencies'):
            dependencies = plugin_module.dependencies
            for dependency in dependencies:
                self.resolve_dependencies(dependency)

    def detect_version_conflicts(self):
        versions = {}
        for plugin_name, plugin_module in self.plugins.items():
            if hasattr(plugin_module, 'version'):
                version = plugin_module.version
                if plugin_name in versions and versions[plugin_name] != version:
                    raise ValueError(f"Version conflict for plugin '{plugin_name}'")
                versions[plugin_name] = version

    def execute_plugin(self, plugin_name):
        plugin_module = self.load_plugin(plugin_name)

        if hasattr(plugin_module, 'execute'):
            plugin_module.execute()

# Usage example
plugins_dir = '/path/to/plugins'
plugin_manager = PluginManager(plugins_dir)
plugin_manager.resolve_dependencies('my_plugin')
plugin_manager.detect_version_conflicts()
plugin_manager.execute_plugin('my_plugin')
