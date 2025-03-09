import subprocess
from multiprocessing import Pool

class BuildProcess:
    def __init__(self):
        self.dependencies = {}

    def add_dependency(self, target, dependencies):
        self.dependencies[target] = dependencies

    def build(self, target):
        if target not in self.dependencies:
            print(f"Target '{target}' does not exist.")
            return

        # Build the dependencies first
        for dependency in self.dependencies[target]:
            self.build(dependency)

        # Build the target
        print(f"Building target '{target}'...")
        subprocess.run(["make", target])

    def build_parallel(self, target):
        if target not in self.dependencies:
            print(f"Target '{target}' does not exist.")
            return

        # Build the dependencies in parallel
        with Pool() as pool:
            pool.map(self.build, self.dependencies[target])

        # Build the target
        print(f"Building target '{target}'...")
        subprocess.run(["make", target])
        if __name__ == "__main__":
            build_process = BuildProcess()
            build_process.add_dependency("target1", ["dependency1", "dependency2"])
            build_process.add_dependency("target2", ["dependency3", "dependency4"])
            build_process.build_parallel("target2")
