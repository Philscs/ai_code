class SafeResourceHandler:
    def __init__(self, resource):
        self.resource = resource

    def __enter__(self):
        print("Resource initialized.")
        return self.resource

    def __exit__(self, exc_type, exc_value, traceback):
        try:
            print("Cleaning up the resource.")
            if self.resource:
                self.resource.close()
        except Exception as cleanup_error:
            print(f"Error during cleanup: {cleanup_error}")
        finally:
            self.resource = None
            print("Resource is fully cleaned up.")

        if exc_type:
            print(f"Exception occurred: {exc_type}, {exc_value}")
            return False
        return True

# Example usage of the Context Manager
if __name__ == "__main__":
    class MockResource:
        def close(self):
            print("MockResource closed.")

        def cleanup_sensitive_data(self):
            print("Sensitive data cleaned up.")

    try:
        with SafeResourceHandler(MockResource()) as resource:
            print("Using the resource.")
            resource.cleanup_sensitive_data()
            # Simulate an exception
            raise ValueError("Something went wrong!")
    except ValueError as e:
        print(f"Caught exception: {e}")
