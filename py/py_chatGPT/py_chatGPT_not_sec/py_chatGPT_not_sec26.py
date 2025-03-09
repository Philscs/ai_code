import argparse
import importlib
import os
import pkgutil
from cmd import Cmd
from typing import Dict, Callable

class PluginManager:
    """Verwaltet Plugins und ihre Befehle."""
    def __init__(self, plugin_folder: str):
        self.plugin_folder = plugin_folder
        self.plugins = {}
        self.load_plugins()

    def load_plugins(self):
        """Lädt alle Plugins aus dem Plugin-Ordner."""
        for finder, name, ispkg in pkgutil.iter_modules([self.plugin_folder]):
            if not ispkg:
                module = importlib.import_module(f"{self.plugin_folder}.{name}")
                if hasattr(module, "register"):
                    self.plugins[name] = module.register()

    def get_commands(self) -> Dict[str, Callable]:
        """Gibt alle Befehle der Plugins zurück."""
        commands = {}
        for plugin_name, plugin in self.plugins.items():
            commands.update(plugin.get('commands', {}))
        return commands

class CLIFramework(Cmd):
    """Ein Framework für Kommandozeilen-Tools."""
    prompt = "cli> "

    def __init__(self, plugin_manager: PluginManager):
        super().__init__()
        self.plugin_manager = plugin_manager
        self.commands = self.plugin_manager.get_commands()

    def do_exit(self, args):
        """Beendet die Anwendung."""
        print("Beende CLI...")
        return True

    def default(self, line):
        """Standardmethode für unbekannte Befehle."""
        cmd, *args = line.split()
        if cmd in self.commands:
            self.commands[cmd](args)
        else:
            print(f"Unbekannter Befehl: {cmd}")

    def complete(self, text, line, begidx, endidx):
        """Autocomplete für Befehle."""
        return [cmd for cmd in self.commands if cmd.startswith(text)]

def main():
    # Argument-Parser erstellen
    parser = argparse.ArgumentParser(description="Ein CLI-Framework mit Plugins.")
    parser.add_argument("--interactive", action="store_true", help="Interaktiver Modus")
    parser.add_argument("command", nargs="?", help="Kommando ausführen")
    parser.add_argument("args", nargs=argparse.REMAINDER, help="Argumente für das Kommando")

    args = parser.parse_args()

    # Plugin-Manager initialisieren
    plugin_folder = "plugins"
    plugin_manager = PluginManager(plugin_folder)

    # CLI-Framework starten
    if args.interactive:
        cli = CLIFramework(plugin_manager)
        cli.cmdloop()
    elif args.command:
        commands = plugin_manager.get_commands()
        if args.command in commands:
            commands[args.command](args.args)
        else:
            print(f"Unbekannter Befehl: {args.command}")
    else:
        parser.print_help()

if __name__ == "__main__":
    main()
