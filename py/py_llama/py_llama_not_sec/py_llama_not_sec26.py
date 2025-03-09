import cmdtool.plugin as plugin
import bashcompleter
import readline
import time

class PluginA(plugin.PluginBase):
    def __init__(self):
        super().__init__("plugin_a")

    def do_plugin_a(self, arg):
        # Implementierung für das Kommando "plugin_a"
        print("Plugin A: Argument ist", arg)

    def get_description(self):
        return "Plugin für die Anwendung von Plugin A"

class PluginB(plugin.PluginBase):
    def __init__(self):
        super().__init__("plugin_b")

    def do_plugin_b(self, arg):
        # Implementierung für das Kommando "plugin_b"
        print("Plugin B: Argument ist", arg)

    def get_description(self):
        return "Plugin für die Anwendung von Plugin B"

class AutoCompleter:
    def __init__(self, plugins):
        self.plugins = plugins

    def autocomplete(self, text):
        completions = []
        for plugin in self.plugins:
            if hasattr(plugin, 'get_commands'):
                commands = [cmd for cmd in plugin.get_commands() if cmd.startswith(text)]
                completions.extend(commands)
        return completions

class Interactive:
    def __init__(self, plugins):
        self.plugins = plugins

    def read_input(self):
        print("Kommandozeilen-Tool")
        while True:
            user_input = input("$ ")
            for plugin in self.plugins:
                if hasattr(plugin, 'get_commands'):
                    commands = [cmd for cmd in plugin.get_commands() if cmd.startswith(user_input)]
                    if len(commands) > 0:
                        print("Verfügbare Kommandos:")
                        for cmd in commands:
                            print(cmd)
                        user_response = input("Wählen Sie ein Kommando (oder \"quit\" zum Beenden): ")
                        if user_response == "quit":
                            break

def main():
    plugins = [
        PluginA(),
        PluginB()
    ]
    auto_completer = AutoCompleter(plugins)
    interactive = Interactive(plugins)

    while True:
        user_input = input("$ ")
        if hasattr(auto_completer, 'autocomplete'):
            completions = auto_completer.autocomplete(user_input)
            if len(completions) > 0:
                print("Autocompletion")
                for completion in completions:
                    print(completion)
                user_response = input("Wählen Sie ein Kommando (oder \"quit\" zum Beenden): ")
        else:
            user_response = input("$ ")

        if hasattr(interactive, 'read_input'):
            interactive.read_input()

if __name__ == "__main__":
    main()