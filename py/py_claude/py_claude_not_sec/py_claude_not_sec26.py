import abc
import argparse
import cmd
import importlib
import inspect
import os
import pkgutil
import readline
import sys
from typing import Dict, List, Optional, Type

class Plugin(abc.ABC):
    """Basis-Klasse für Plugins"""
    
    @abc.abstractmethod
    def get_name(self) -> str:
        """Gibt den Namen des Plugins zurück"""
        pass
    
    @abc.abstractmethod
    def get_description(self) -> str:
        """Gibt die Beschreibung des Plugins zurück"""
        pass
    
    @abc.abstractmethod
    def execute(self, args: argparse.Namespace) -> None:
        """Führt die Plugin-Logik aus"""
        pass

    @abc.abstractmethod
    def setup_parser(self, parser: argparse.ArgumentParser) -> None:
        """Konfiguriert den ArgumentParser für das Plugin"""
        pass

class PluginManager:
    """Verwaltet die Plugin-Registrierung und -Ausführung"""
    
    def __init__(self, plugin_dir: str = "plugins"):
        self.plugin_dir = plugin_dir
        self.plugins: Dict[str, Type[Plugin]] = {}
        self._load_plugins()
    
    def _load_plugins(self) -> None:
        """Lädt alle Plugins aus dem Plugin-Verzeichnis"""
        if not os.path.exists(self.plugin_dir):
            os.makedirs(self.plugin_dir)
            
        sys.path.insert(0, self.plugin_dir)
        
        for _, name, _ in pkgutil.iter_modules([self.plugin_dir]):
            module = importlib.import_module(name)
            
            for item_name, item in inspect.getmembers(module):
                if (inspect.isclass(item) and 
                    issubclass(item, Plugin) and 
                    item != Plugin):
                    plugin_instance = item()
                    self.plugins[plugin_instance.get_name()] = item

    def get_plugin(self, name: str) -> Optional[Type[Plugin]]:
        """Gibt ein Plugin anhand seines Namens zurück"""
        return self.plugins.get(name)
    
    def get_all_plugins(self) -> List[Type[Plugin]]:
        """Gibt alle registrierten Plugins zurück"""
        return list(self.plugins.values())

class InteractiveShell(cmd.Cmd):
    """Interaktive Shell für das CLI-Framework"""
    
    intro = 'Willkommen im interaktiven Modus. Geben Sie "help" oder "?" ein für Hilfe.\n'
    prompt = '> '
    
    def __init__(self, plugin_manager: PluginManager):
        super().__init__()
        self.plugin_manager = plugin_manager
        self._setup_completions()
    
    def _setup_completions(self) -> None:
        """Konfiguriert die Auto-Completion"""
        plugin_names = list(self.plugin_manager.plugins.keys())
        readline.set_completer_delims(' \t\n;')
        readline.parse_and_bind("tab: complete")
        
        def complete(text: str, state: int) -> Optional[str]:
            if not text:
                completions = plugin_names[:]
            else:
                completions = [p for p in plugin_names if p.startswith(text)]
            
            try:
                return completions[state]
            except IndexError:
                return None
        
        readline.set_completer(complete)
    
    def do_exit(self, arg: str) -> bool:
        """Beendet den interaktiven Modus"""
        return True
    
    def default(self, line: str) -> None:
        """Verarbeitet unbekannte Befehle"""
        args = line.split()
        if args:
            plugin = self.plugin_manager.get_plugin(args[0])
            if plugin:
                parser = argparse.ArgumentParser(description=plugin().get_description())
                plugin().setup_parser(parser)
                try:
                    parsed_args = parser.parse_args(args[1:])
                    plugin().execute(parsed_args)
                except SystemExit:
                    pass  # Verhindert, dass argparse die Shell beendet
            else:
                print(f"Unbekannter Befehl: {args[0]}")

class CLIFramework:
    """Hauptklasse des CLI-Frameworks"""
    
    def __init__(self, plugin_dir: str = "plugins"):
        self.plugin_manager = PluginManager(plugin_dir)
    
    def run(self, args: Optional[List[str]] = None) -> None:
        """Startet das Framework im Batch- oder interaktiven Modus"""
        if args is None:
            args = sys.argv[1:]
        
        if not args or args[0] == "--interactive":
            # Interaktiver Modus
            InteractiveShell(self.plugin_manager).cmdloop()
        else:
            # Batch-Modus
            parser = argparse.ArgumentParser(description="CLI-Framework")
            subparsers = parser.add_subparsers(dest="plugin", help="Verfügbare Plugins")
            
            # Registriere alle Plugin-Subparser
            for plugin_name, plugin_class in self.plugin_manager.plugins.items():
                plugin = plugin_class()
                subparser = subparsers.add_parser(
                    plugin_name, 
                    help=plugin.get_description()
                )
                plugin.setup_parser(subparser)
            
            # Parse Argumente und führe das entsprechende Plugin aus
            parsed_args = parser.parse_args(args)
            if parsed_args.plugin:
                plugin = self.plugin_manager.get_plugin(parsed_args.plugin)
                if plugin:
                    plugin().execute(parsed_args)
            else:
                parser.print_help()

# Beispiel für ein konkretes Plugin
class HelloPlugin(Plugin):
    """Beispiel-Plugin, das eine Begrüßung ausgibt"""
    
    def get_name(self) -> str:
        return "hello"
    
    def get_description(self) -> str:
        return "Gibt eine personalisierte Begrüßung aus"
    
    def setup_parser(self, parser: argparse.ArgumentParser) -> None:
        parser.add_argument("--name", default="Welt", help="Name für die Begrüßung")
    
    def execute(self, args: argparse.Namespace) -> None:
        print(f"Hallo, {args.name}!")

# Beispiel für die Verwendung des Frameworks
if __name__ == "__main__":
    # Erstelle das Plugin-Verzeichnis und speichere das Hello-Plugin
    os.makedirs("plugins", exist_ok=True)
    with open("plugins/hello_plugin.py", "w") as f:
        f.write(inspect.getsource(HelloPlugin))
    
    # Starte das Framework
    framework = CLIFramework()
    framework.run()