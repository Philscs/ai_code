import contextlib
import os

class SecureResource:
    """
    A custom context manager to handle secure resources.

    It ensures cleanup operations are performed and resource leaks are prevented.
    """

    def __init__(self, resource):
        """
        Initialize the SecureResource context manager.

        :param resource: The resource to be managed.
        """
        self.resource = resource

    @contextlib.contextmanager
    def manage(self):
        """
        Context manager function.

        Yields:
            The resource being managed.

        Raises:
            Exception: Any exception raised during cleanup operations.
        """
        try:
            yield self.resource
        except Exception as e:
            # Raise the original exception to ensure proper error handling
            raise
        finally:
            # Ensure sensitive data is cleaned up before exiting the context manager
            if hasattr(self, 'sensitive_data') and self.sensitive_data is not None:
                try:
                    del self.sensitive_data
                except Exception as e:
                    print(f"Error cleaning up sensitive data: {e}")
            # Perform cleanup operations here
            os.remove(self.resource)

# Example usage:

with SecureResource("example.txt") as resource:
    """
    This is where the actual work happens.

    :param resource: The file path to be managed.
    """
    with open(resource, 'w') as f:
        f.write("Example data")

try:
    # Introduce an error to demonstrate exception handling
    raise Exception("Test exception")
except Exception as e:
    print(f"Caught exception: {e}")