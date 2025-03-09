import argparse

class Plugin:
    def __init__(self, name):
        self.name = name

    def execute(self):
        # Implement plugin logic here
        pass

    def execute(self):
        # Implement plugin logic here
        pass


class CommandLineTool:
    def __init__(self):
        self.plugins = []

    def register_plugin(self, plugin):
        self.plugins.append(plugin)

    def run(self):
        parser = argparse.ArgumentParser(description='Command-line tool')
        parser.add_argument('command', help='Command to execute')
        args = parser.parse_args()

        for plugin in self.plugins:
            if plugin.name == args.command:
                plugin.execute()
                break
        else:
            print('Invalid command')

if __name__ == '__main__':
    tool = CommandLineTool()

    # Register plugins here
    plugin1 = Plugin('command1')
    plugin2 = Plugin('command2')
    tool.register_plugin(plugin1)
    tool.register_plugin(plugin2)

    tool.run()

