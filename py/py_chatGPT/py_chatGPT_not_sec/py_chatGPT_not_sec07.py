import os
import pickle
from datetime import datetime
from collections import defaultdict

class VersionControlSystem:
    def __init__(self):
        self.repo = defaultdict(dict)  # {branch_name: {file_name: content}}
        self.branches = ['main']
        self.current_branch = 'main'
        self.history = []  # List of commits
        self.unstaged_changes = {}  # {file_name: content}
        self.binary_files = {}  # {file_name: binary_data}

    def stage_change(self, file_name, content, is_binary=False):
        if is_binary:
            self.binary_files[file_name] = content
        else:
            self.unstaged_changes[file_name] = content

    def commit(self, message):
        timestamp = datetime.now().isoformat()
        commit_id = len(self.history) + 1

        # Apply changes
        self.repo[self.current_branch].update(self.unstaged_changes)
        for file_name, content in self.binary_files.items():
            self.repo[self.current_branch][file_name] = content

        self.history.append({
            'id': commit_id,
            'branch': self.current_branch,
            'message': message,
            'timestamp': timestamp,
            'changes': {
                'text_files': self.unstaged_changes.copy(),
                'binary_files': list(self.binary_files.keys()),
            },
        })

        self.unstaged_changes.clear()
        self.binary_files.clear()
        print(f"Committed changes to {self.current_branch} with ID {commit_id}.")

    def create_branch(self, branch_name):
        if branch_name in self.branches:
            print(f"Branch {branch_name} already exists.")
        else:
            self.repo[branch_name] = self.repo[self.current_branch].copy()
            self.branches.append(branch_name)
            print(f"Branch {branch_name} created.")

    def switch_branch(self, branch_name):
        if branch_name not in self.branches:
            print(f"Branch {branch_name} does not exist.")
        else:
            self.current_branch = branch_name
            print(f"Switched to branch {branch_name}.")

    def merge_branch(self, source_branch):
        if source_branch not in self.branches:
            print(f"Branch {source_branch} does not exist.")
            return

        conflicts = []
        source_files = self.repo[source_branch]
        current_files = self.repo[self.current_branch]

        for file_name, content in source_files.items():
            if file_name in current_files and current_files[file_name] != content:
                conflicts.append(file_name)
            else:
                self.repo[self.current_branch][file_name] = content

        if conflicts:
            print(f"Merge completed with conflicts: {conflicts}")
        else:
            print("Merge completed successfully.")

    def view_history(self):
        for commit in self.history:
            print(f"ID: {commit['id']}, Branch: {commit['branch']}, Message: {commit['message']}, Timestamp: {commit['timestamp']}")

    def save_repo(self, file_path):
        with open(file_path, 'wb') as f:
            pickle.dump({
                'repo': self.repo,
                'branches': self.branches,
                'current_branch': self.current_branch,
                'history': self.history,
            }, f)
        print(f"Repository saved to {file_path}.")

    def load_repo(self, file_path):
        if os.path.exists(file_path):
            with open(file_path, 'rb') as f:
                data = pickle.load(f)
                self.repo = data['repo']
                self.branches = data['branches']
                self.current_branch = data['current_branch']
                self.history = data['history']
            print(f"Repository loaded from {file_path}.")
        else:
            print(f"File {file_path} does not exist.")

# Example usage
vcs = VersionControlSystem()
vcs.stage_change("file1.txt", "Initial content")
vcs.commit("Initial commit")
vcs.create_branch("feature")
vcs.switch_branch("feature")
vcs.stage_change("file2.txt", "Feature content")
vcs.commit("Added feature")
vcs.switch_branch("main")
vcs.merge_branch("feature")
vcs.view_history()
