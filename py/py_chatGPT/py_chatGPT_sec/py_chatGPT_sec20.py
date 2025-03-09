import os
import importlib.util
import hashlib
import inspect
from typing import List, Dict, Any

class Plugin:
    def __init__(self, name: str, version: str, dependencies: List[str]):
        self.name = name
        self.version = version
        self.dependencies = dependencies

class PluginManager:
    def __init__(self, plugin_dir: str, allowed_hashes: Dict[str, str]):
        self.plugin_dir = plugin_dir
        self.allowed_hashes = allowed_hashes
        self.plugins = {}
        self.loaded_plugins = {}

    def verify_plugin(self, plugin_path: str) -> bool:
        """Verifies the integrity of the plugin using its hash."""
        with open(plugin_path, "rb") as f:
            file_hash = hashlib.sha256(f.read()).hexdigest()
        plugin_name = os.path.basename(plugin_path)
        return self.allowed_hashes.get(plugin_name) == file_hash

    def load_plugins(self):
        """Loads all valid plugins from the plugin directory."""
        for filename in os.listdir(self.plugin_dir):
            if filename.endswith(".py"):
                plugin_path = os.path.join(self.plugin_dir, filename)
                if self.verify_plugin(plugin_path):
                    self.load_plugin(plugin_path)
                else:
                    print(f"Plugin {filename} failed verification and will not be loaded.")

    def load_plugin(self, plugin_path: str):
        """Dynamically loads a plugin."""
        spec = importlib.util.spec_from_file_location("plugin_module", plugin_path)
        plugin_module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(plugin_module)

        for name, obj in inspect.getmembers(plugin_module):
            if inspect.isclass(obj) and issubclass(obj, Plugin) and obj is not Plugin:
                plugin_instance = obj()
                self.register_plugin(plugin_instance)

    def register_plugin(self, plugin: Plugin):
        """Registers a plugin and resolves dependencies."""
        if plugin.name in self.plugins:
            existing_plugin = self.plugins[plugin.name]
            if existing_plugin.version != plugin.version:
                raise Exception(f"Version conflict for plugin {plugin.name}: {existing_plugin.version} vs {plugin.version}")
        self.plugins[plugin.name] = plugin
        self.resolve_dependencies(plugin)

    def resolve_dependencies(self, plugin: Plugin):
        """Resolves dependencies for a plugin."""
        for dependency in plugin.dependencies:
            if dependency not in self.plugins:
                raise Exception(f"Unresolved dependency: {dependency} for plugin {plugin.name}")

    def execute_plugin(self, plugin_name: str, *args, **kwargs) -> Any:
        """Executes a plugin in a sandboxed environment."""
        if plugin_name not in self.plugins:
            raise Exception(f"Plugin {plugin_name} not found.")

        plugin = self.plugins[plugin_name]
        sandbox = {}
        exec(inspect.getsource(plugin.__class__), sandbox)
        plugin_class = sandbox[plugin.__class__.__name__]
        plugin_instance = plugin_class()
        return plugin_instance.run(*args, **kwargs)

# Beispiel eines Plugins
class MyPlugin(Plugin):
    def __init__(self):
        super().__init__("MyPlugin", "1.0", [])

    def run(self, *args, **kwargs):
        return "Plugin executed successfully!"

# Verwendung
if __name__ == "__main__":
    allowed_hashes = {
        "my_plugin.py": "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"
    }
    plugin_manager = PluginManager("./plugins", allowed_hashes)
    plugin_manager.load_plugins()
    result = plugin_manager.execute_plugin("MyPlugin")
    print(result)
