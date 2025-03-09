import subprocess
import shlex
import os
import signal

class SecureSubprocess:
    def __init__(self, command, timeout=None, max_memory=None, env=None):
        self.command = command
        self.timeout = timeout
        self.max_memory = max_memory
        self.env = env

    def run(self):
        # Prepare the command
        command_args = shlex.split(self.command)
        executable = command_args[0]
        args = command_args[1:]

        # Set environment variables
        env = os.environ.copy()
        if self.env:
            env.update(self.env)

        # Create the subprocess
        process = subprocess.Popen(
            [executable] + args,
            env=env,
            preexec_fn=self._set_limits,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )

        # Set a timeout if specified
        if self.timeout:
            signal.signal(signal.SIGALRM, self._timeout_handler)
            signal.alarm(self.timeout)

        # Wait for the process to complete
        stdout, stderr = process.communicate()

        # Reset the timeout
        if self.timeout:
            signal.alarm(0)

        # Return the result
        return process.returncode, stdout.decode(), stderr.decode()

    def _set_limits(self):
        # Set resource limits if specified
        if self.max_memory:
            resource.setrlimit(resource.RLIMIT_AS, (self.max_memory, self.max_memory))

    def _timeout_handler(self, signum, frame):
        # Timeout handler
        raise TimeoutError("Command execution timed out")

# Example usage
command = "ls -l"
wrapper = SecureSubprocess(command, timeout=5, max_memory=1024)
returncode, stdout, stderr = wrapper.run()
print(f"Return code: {returncode}")
print(f"Standard output: {stdout}")
print(f"Standard error: {stderr}")
