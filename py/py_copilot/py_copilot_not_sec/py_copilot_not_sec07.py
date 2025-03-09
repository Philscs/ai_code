class Commit:
    def __init__(self, message, parent=None):
        self.message = message
        self.parent = parent

class Branch:
    def __init__(self, name, commit):
        self.name = name
        self.commit = commit

class VersionControlSystem:
    def __init__(self):
        self.branches = {}
        self.current_branch = None

    def create_branch(self, name, commit):
        branch = Branch(name, commit)
        self.branches[name] = branch

    def switch_branch(self, name):
        if name in self.branches:
            self.current_branch = self.branches[name]
        else:
            print(f"Branch '{name}' does not exist.")

    def commit(self, message):
        if self.current_branch is None:
            print("No branch selected.")
            return

        commit = Commit(message, self.current_branch.commit)
        self.current_branch.commit = commit

    def print_commit_history(self):
        if self.current_branch is None:
            print("No branch selected.")
            return

        commit = self.current_branch.commit
        while commit is not None:
            print(commit.message)
            commit = commit.parent

# Beispielverwendung
vcs = VersionControlSystem()
vcs.create_branch("main", Commit("Initial commit"))
vcs.switch_branch("main")
vcs.commit("Add feature A")
vcs.commit("Fix bug B")
vcs.print_commit_history()
