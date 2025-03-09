import os
import subprocess
from collections import defaultdict, deque
from typing import Dict, List

class BuildProcess:
    def __init__(self, name: str):
        self.name = name
        self.dependencies = []

    def add_dependency(self, build_process: 'BuildProcess'):
        self.dependencies.append(build_process)


class BuildManager:
    def __init__(self):
        self.build_processes = {}
        self.locks = {}

    def create_build_process(self, name: str):
        if name in self.build_processes:
            raise ValueError(f"Build process '{name}' already exists")
        build_process = BuildProcess(name)
        self.build_processes[name] = build_process
        return build_process

    def add_dependency(self, build_process_name: str, dependency_build_process_name: str):
        if build_process_name not in self.build_processes:
            raise ValueError(f"Build process '{build_process_name}' does not exist")
        if dependency_build_process_name not in self.build_processes:
            raise ValueError(f"Build process '{dependency_build_process_name}' does not exist")
        build_process = self.build_processes[build_process_name]
        build_process.add_dependency(self.build_processes[dependency_build_process_name])

    def run_build_process(self, build_process_name: str):
        if build_process_name not in self.build_processes:
            raise ValueError(f"Build process '{build_process_name}' does not exist")
        build_process = self.build_processes[build_process_name]
        print(f"Running build process '{build_process_name}'")
        for dependency_build_process in build_process.dependencies:
            self.run_build_process(dependency_build_process.name)
        command = f"make -j8 {build_process_name}"  # Hier sollte der Befehl angepasst werden
        os.system(command)

    def create_lock(self, build_process_name: str):
        if build_process_name not in self.locks:
            self.locks[build_process_name] = False

    def lock_build_process(self, build_process_name: str):
        if build_process_name not in self.build_processes or build_process_name not in self.locks:
            raise ValueError(f"Build process '{build_process_name}' does not exist")
        if not self.locks[build_process_name]:
            self.create_lock(build_process_name)
            self.locks[build_process_name] = True
            print(f"Lock acquired for build process '{build_process_name}'")

    def unlock_build_process(self, build_process_name: str):
        if build_process_name not in self.build_processes or build_process_name not in self.locks:
            raise ValueError(f"Build process '{build_process_name}' does not exist")
        if self.locks[build_process_name]:
            self.locks[build_process_name] = False
            print(f"Lock released for build process '{build_process_name}'")

# Beispiel für die Verwendung des Systems
if __name__ == "__main__":
    build_manager = BuildManager()

    # Erstellen von Build-Prozessen
    web_server_build_process = build_manager.create_build_process("web_server")
    database_build_process = build_manager.create_build_process("database")

    # Hinzufügen von Abhängigkeiten
    web_server_build_process.add_dependency(database_build_process)

    # Runnen der Build-Prozesse
    build_manager.run_build_process("web_server")