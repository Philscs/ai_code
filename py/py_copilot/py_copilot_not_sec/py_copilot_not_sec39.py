import subprocess

def run_static_analysis(file_path):
    # Run static analysis tool (e.g., pylint) on the given file
    result = subprocess.run(['pylint', file_path], capture_output=True, text=True)
    return result.stdout

def run_style_checking(file_path):
    # Run style checking tool (e.g., flake8) on the given file
    result = subprocess.run(['flake8', file_path], capture_output=True, text=True)
    return result.stdout

def perform_code_review(file_path):
    # Run static analysis
    static_analysis_result = run_static_analysis(file_path)
    print("Static Analysis Result:")
    print(static_analysis_result)

    # Run style checking
    style_checking_result = run_style_checking(file_path)
    print("Style Checking Result:")
    print(style_checking_result)

# Example usage
file_path = '/path/to/your/file.py'
perform_code_review(file_path)
