import logging
import logging
import traceback
import json
from datetime import datetime
import sys

class CustomException(Exception):
    pass

def clean_stack_trace(stack_trace):
    # Clean up the stack trace by removing internal system information
    # You can customize this function based on your requirements
    cleaned_trace = []
    for frame in stack_trace:
        if 'internal_module' not in frame.filename:
            cleaned_trace.append(frame)
    return cleaned_trace

def filter_sensitive_info(error_message):
    # Filter sensitive information from the error message
    # You can customize this function based on your requirements
    filtered_message = error_message.replace('sensitive_info', '***')
    return filtered_message

def create_error_log(exception, stack_trace):
    # Create a structured error log
    # You can customize this function based on your requirements
    error_log = {
        'exception_type': type(exception).__name__,
        'error_message': str(exception),
        'stack_trace': stack_trace
    }
    return error_log

def custom_exception_handler(exception_type, exception, stack_trace):
    # Clean up the stack trace
    cleaned_trace = clean_stack_trace(stack_trace)

    # Filter sensitive information from the error message
    filtered_message = filter_sensitive_info(str(exception))

    # Create a structured error log
    error_log = create_error_log(exception, cleaned_trace)

    # Log the error
    logging.error(error_log)

    # Print a user-friendly error message
    print(f"An error occurred: {filtered_message}")

# Register the custom exception handler
sys.excepthook = custom_exception_handler

# Example usage
try:
    # Your code here
    raise CustomException("Sensitive information leaked!")
except CustomException as e:
    # The custom exception handler will be triggered
    pass

    class SensitiveInfoFilter(logging.Filter):
        def filter(self, record):
            if hasattr(record, 'message') and isinstance(record.message, str):
                # Example: Filtering sensitive information
                record.message = record.message.replace('SECRET', '***')
            return True

    def clean_stack_trace(stack_trace):
        """
        Cleans up the stack trace by removing specific system information.
        """
        clean_lines = []
        for line in stack_trace.splitlines():
            if "sensitive_path" not in line:  # Example: Filtering paths
                clean_lines.append(line)
        return "\n".join(clean_lines)

    def create_structured_error_log(exc_type, exc_value, tb):
        """
        Creates a structured error log.
        """
        stack_trace = traceback.format_exception(exc_type, exc_value, tb)
        cleaned_trace = clean_stack_trace("".join(stack_trace))

        error_log = {
            "timestamp": datetime.utcnow().isoformat(),
            "error_type": exc_type.__name__,
            "error_message": str(exc_value).replace('SECRET', '***'),  # Filter sensitive data
            "stack_trace": cleaned_trace
        }
        return error_log

    def custom_exception_handler(exc_type, exc_value, tb):
        """
        Custom exception handler that handles the errors.
        """
        # Prepare logging
        logger = logging.getLogger("CustomExceptionHandler")
        logger.setLevel(logging.ERROR)

        # Add filter
        sensitive_filter = SensitiveInfoFilter()
        logger.addFilter(sensitive_filter)

        # Output format
        handler = logging.StreamHandler()
        handler.setFormatter(logging.Formatter('%(asctime)s - %(levelname)s - %(message)s'))
        logger.addHandler(handler)

        # Create error log
        structured_log = create_structured_error_log(exc_type, exc_value, tb)

        # Log error as JSON
        logger.error(json.dumps(structured_log, indent=4))

    # Example usage of the handler
    if __name__ == "__main__":

        sys.excepthook = custom_exception_handler

        # Example of an error with sensitive information
        SECRET = "12345"
        raise ValueError(f"An error with sensitive data: {SECRET}")
