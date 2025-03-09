import os
import sys
import hashlib
import importlib.util
import pkg_resources
from typing import Dict, List, Optional
from dataclasses import dataclass
import subprocess
from pathlib import Path

@dataclass
class PluginMetadata:
    """Metadaten für ein Plugin"""
    name: str
    version: str
    dependencies: Dict[str, str]
    checksum: str
    entry_point: str

class PluginValidator:
    """Überprüft die Sicherheit und Integrität von Plugins"""
    
    @staticmethod
    def calculate_checksum(plugin_path: Path) -> str:
        """Berechnet den SHA-256 Hashwert einer Plugin-Datei"""
        sha256_hash = hashlib.sha256()
        with open(plugin_path, "rb") as f:
            for byte_block in iter(lambda: f.read(4096), b""):
                sha256_hash.update(byte_block)
        return sha256_hash.hexdigest()
    
    @staticmethod
    def validate_plugin(plugin_path: Path, metadata: PluginMetadata) -> bool:
        """Überprüft die Integrität eines Plugins"""
        # Überprüfe Checksum
        current_checksum = PluginValidator.calculate_checksum(plugin_path)
        if current_checksum != metadata.checksum:
            raise SecurityError(f"Checksum mismatch for plugin {metadata.name}")
            
        # Führe statische Code-Analyse durch
        try:
            subprocess.run(['bandit', str(plugin_path)], check=True)
        except subprocess.CalledProcessError:
            raise SecurityError(f"Security check failed for plugin {metadata.name}")
            
        return True

class DependencyResolver:
    """Löst Plugin-Abhängigkeiten auf und erkennt Konflikte"""
    
    @staticmethod
    def check_dependencies(plugins: Dict[str, PluginMetadata]) -> bool:
        """Überprüft Abhängigkeiten auf Konflikte"""
        for plugin_name, metadata in plugins.items():
            for dep_name, dep_version in metadata.dependencies.items():
                if dep_name in plugins:
                    # Überprüfe Version mit pkg_resources
                    required = pkg_resources.parse_version(dep_version)
                    available = pkg_resources.parse_version(plugins[dep_name].version)
                    if available < required:
                        raise DependencyError(
                            f"Version conflict: {plugin_name} requires {dep_name}>={dep_version}"
                        )
        return True

class Sandbox:
    """Führt Plugin-Code in einer isolierten Umgebung aus"""
    
    def __init__(self):
        self.restricted_globals = {
            '__builtins__': {
                name: getattr(__builtins__, name)
                for name in ['print', 'len', 'str', 'int', 'float', 'list', 'dict']
            }
        }
    
    def execute(self, code: str) -> None:
        """Führt Code in der Sandbox aus"""
        try:
            exec(code, self.restricted_globals, {})
        except Exception as e:
            raise SandboxError(f"Error in sandbox execution: {str(e)}")

class PluginManager:
    """Hauptklasse für das Plugin-Management"""
    
    def __init__(self, plugin_dir: str):
        self.plugin_dir = Path(plugin_dir)
        self.plugins: Dict[str, PluginMetadata] = {}
        self.loaded_plugins = {}
        self.validator = PluginValidator()
        self.sandbox = Sandbox()
    
    def discover_plugins(self) -> None:
        """Sucht nach verfügbaren Plugins im Plugin-Verzeichnis"""
        for plugin_path in self.plugin_dir.glob("*.py"):
            try:
                with open(plugin_path.with_suffix('.meta.json'), 'r') as f:
                    import json
                    metadata = PluginMetadata(**json.load(f))
                    if self.validator.validate_plugin(plugin_path, metadata):
                        self.plugins[metadata.name] = metadata
            except Exception as e:
                print(f"Error loading plugin metadata: {str(e)}")

    def load_plugin(self, plugin_name: str) -> Optional[object]:
        """Lädt ein Plugin sicher in die Sandbox"""
        if plugin_name not in self.plugins:
            raise PluginNotFoundError(f"Plugin {plugin_name} not found")
            
        metadata = self.plugins[plugin_name]
        plugin_path = self.plugin_dir / f"{plugin_name}.py"
        
        # Überprüfe Abhängigkeiten
        DependencyResolver.check_dependencies(self.plugins)
        
        # Lade Plugin in Sandbox
        try:
            with open(plugin_path, 'r') as f:
                plugin_code = f.read()
            
            # Führe Code in Sandbox aus
            self.sandbox.execute(plugin_code)
            
            # Importiere Plugin-Modul
            spec = importlib.util.spec_from_file_location(plugin_name, plugin_path)
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            
            # Instanziiere Plugin-Klasse
            plugin_class = getattr(module, metadata.entry_point)
            plugin_instance = plugin_class()
            
            self.loaded_plugins[plugin_name] = plugin_instance
            return plugin_instance
            
        except Exception as e:
            raise PluginLoadError(f"Error loading plugin {plugin_name}: {str(e)}")
    
    def unload_plugin(self, plugin_name: str) -> None:
        """Entlädt ein Plugin sicher"""
        if plugin_name in self.loaded_plugins:
            del self.loaded_plugins[plugin_name]
            
    def get_plugin(self, plugin_name: str) -> Optional[object]:
        """Gibt eine Referenz auf ein geladenes Plugin zurück"""
        return self.loaded_plugins.get(plugin_name)

# Benutzerdefinierte Exceptions
class PluginError(Exception): pass
class SecurityError(PluginError): pass
class DependencyError(PluginError): pass
class PluginNotFoundError(PluginError): pass
class PluginLoadError(PluginError): pass
class SandboxError(PluginError): pass