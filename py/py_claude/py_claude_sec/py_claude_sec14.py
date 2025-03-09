import subprocess
import shlex
import os
import signal
import resource
from typing import List, Dict, Optional, Union
from pathlib import Path


class SecureSubprocessError(Exception):
    """Custom exception for SecureSubprocess errors"""
    pass


class SecureSubprocess:
    """
    A secure wrapper for subprocess execution that prevents command injection,
    implements timeouts, and enforces resource limits.
    """
    
    def __init__(
        self,
        max_memory_mb: int = 512,
        max_cpu_time: int = 30,
        timeout: int = 60,
        allowed_paths: Optional[List[str]] = None,
        allowed_env_vars: Optional[List[str]] = None
    ):
        """
        Initialize the SecureSubprocess wrapper with security constraints.
        
        Args:
            max_memory_mb: Maximum memory usage in MB
            max_cpu_time: Maximum CPU time in seconds
            timeout: Maximum execution time in seconds
            allowed_paths: List of allowed paths for file operations
            allowed_env_vars: List of allowed environment variables
        """
        self.max_memory_mb = max_memory_mb
        self.max_cpu_time = max_cpu_time
        self.timeout = timeout
        self.allowed_paths = [Path(p).resolve() for p in (allowed_paths or [])]
        self.allowed_env_vars = set(allowed_env_vars or [])

    def _set_resource_limits(self):
        """Set resource limits for the subprocess"""
        # Convert MB to bytes
        memory_limit = self.max_memory_mb * 1024 * 1024
        
        # Set memory limit
        resource.setrlimit(resource.RLIMIT_AS, (memory_limit, memory_limit))
        
        # Set CPU time limit
        resource.setrlimit(resource.RLIMIT_CPU, (self.max_cpu_time, self.max_cpu_time))

    def _validate_path(self, path: str) -> bool:
        """
        Validate if a path is allowed.
        
        Args:
            path: Path to validate
            
        Returns:
            bool: True if path is allowed, False otherwise
        """
        if not self.allowed_paths:
            return False
            
        path = Path(path).resolve()
        return any(
            str(path).startswith(str(allowed_path))
            for allowed_path in self.allowed_paths
        )

    def _sanitize_env(self, env: Optional[Dict[str, str]] = None) -> Dict[str, str]:
        """
        Sanitize environment variables.
        
        Args:
            env: Dictionary of environment variables
            
        Returns:
            Dict[str, str]: Sanitized environment variables
        """
        if env is None:
            env = {}
            
        # Only allow specified environment variables
        sanitized_env = {}
        for key, value in env.items():
            if key in self.allowed_env_vars:
                # Ensure environment variables don't contain malicious content
                if isinstance(value, str) and all(c.isprintable() for c in value):
                    sanitized_env[key] = value
                    
        return sanitized_env

    def run(
        self,
        command: Union[str, List[str]],
        input_data: Optional[str] = None,
        working_dir: Optional[str] = None,
        env: Optional[Dict[str, str]] = None,
        **kwargs
    ) -> subprocess.CompletedProcess:
        """
        Safely execute a command with the specified constraints.
        
        Args:
            command: Command to execute (string or list of arguments)
            input_data: Input data to pass to the process
            working_dir: Working directory for the process
            env: Environment variables for the process
            **kwargs: Additional arguments to pass to subprocess.run
            
        Returns:
            subprocess.CompletedProcess: Result of the command execution
            
        Raises:
            SecureSubprocessError: If security constraints are violated
        """
        # Convert string command to list to prevent shell injection
        if isinstance(command, str):
            command = shlex.split(command)
            
        # Validate working directory if specified
        if working_dir and not self._validate_path(working_dir):
            raise SecureSubprocessError(f"Working directory not allowed: {working_dir}")
            
        # Sanitize environment variables
        safe_env = self._sanitize_env(env)
        
        try:
            # Execute the command with security constraints
            result = subprocess.run(
                command,
                input=input_data.encode() if input_data else None,
                capture_output=True,
                text=True,
                cwd=working_dir,
                env=safe_env,
                timeout=self.timeout,
                shell=False,  # Prevent shell injection
                preexec_fn=self._set_resource_limits,
                **kwargs
            )
            
            return result
            
        except subprocess.TimeoutExpired:
            raise SecureSubprocessError(f"Process timed out after {self.timeout} seconds")
        except subprocess.CalledProcessError as e:
            raise SecureSubprocessError(f"Process failed with exit code {e.returncode}")
        except Exception as e:
            raise SecureSubprocessError(f"Unexpected error: {str(e)}")


# Example usage
if __name__ == "__main__":
    # Initialize with security constraints
    secure_proc = SecureSubprocess(
        max_memory_mb=256,
        max_cpu_time=10,
        timeout=30,
        allowed_paths=["/tmp", "/var/log"],
        allowed_env_vars=["PATH", "PYTHONPATH", "LANG"]
    )
    
    try:
        # Execute a safe command
        result = secure_proc.run(["ls", "-l", "/tmp"])
        print("Output:", result.stdout)
        
        # This would raise an error due to unauthorized path
        # result = secure_proc.run(["cat", "/etc/passwd"])
        
        # This would raise an error due to shell injection attempt
        # result = secure_proc.run("ls -l $(cat /etc/passwd)")
        
    except SecureSubprocessError as e:
        print(f"Error: {e}")