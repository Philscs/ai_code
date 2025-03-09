import os
from git import Repo
from subprocess import run
import hashlib

class GitVersionControl:
    def __init__(self, path):
        self.path = path
        if not os.path.exists(self.path):
            os.makedirs(self.path)
        try:
           Repo.init(path)
        except Exception as e:
            print(f"Fehler beim Initieren des Repository: {e}")
            exit(1)

    def add_file(self, file_name, file_content):
        repo = Repo(self.path)
        repo.index.add([file_name])
        commit_hash = repo.index.commit("Add " + file_name, author="Author", 
committer="Committer")
        return commit_hash

    def branch(self, name):
        repo = Repo(self.path)
        try:
            branch = repo.create_branch(name)
            print(f"Branch {name} erstellt")
            return branch
        except Exception as e:
            print(f"Fehler beim Erstellen des Branches: {e}")
            exit(1)

    def merge(self, source_branch, target_branch):
        repo = Repo(self.path)
        try:
            merged_branch = repo.merge(source_branch, target_branch)
            print(f"Branches {source_branch} und {target_branch} in {target_branch}")
            return merged_branch
        except Exception as e:
            print(f"Fehler beim Merging der Branches: {e}")
            exit(1)

    def commit(self, file_name):
        repo = Repo(self.path)
        try:
            commit_hash = repo.index.commit("Commit " + file_name, author="Author", 
committer="Committer")
            return commit_hash
        except Exception as e:
            print(f"Fehler beim Commit: {e}")
            exit(1)

    def create_binary_file(self, filename):
        binary_content = os.urandom(1024)
        sha = self.add_file(filename, binary_content)
        return sha

    def get_commit_history(self):
        repo = Repo(self.path)
        commits = list(repo.iter_commits())
        for commit in commits:
            print(f"Commit: {commit.hexsha} | Author: {commit.author.name} | Date: 
{commit.authored_date}")

# Erstelle ein Git-Versionkontrollsystem
git = GitVersionControl("path/to/your/repository")

# Füge eine Datei hinzu und erstelle einen Commit
file_name = "example.txt"
binary_content = os.urandom(1024)
sha = git.add_file(file_name, binary_content)

# Erstelle einen neuen Branch
new_branch = git.branch("new-branch")
print(f"Branch 'new-branch' erstellt")

# Mische den neuen Branch in den aktuellen Branch
git.merge(new_branch, "main")

# Lade die Commit-Historie
git.get_commit_history()