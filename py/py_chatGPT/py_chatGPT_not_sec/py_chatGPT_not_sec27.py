import json
import inspect
from typing import Callable, Dict, Any, Optional

class APIDocumentationSystem:
    def __init__(self):
        self.endpoints = {}

    def register_endpoint(self, protocol: str, path: str, handler: Callable, auth_method: str, description: str):
        """
        Register an API endpoint with its protocol, path, authentication method, and handler.
        """
        if protocol not in self.endpoints:
            self.endpoints[protocol] = []

        self.endpoints[protocol].append({
            "path": path,
            "handler": handler,
            "auth_method": auth_method,
            "description": description,
            "parameters": self._get_function_params(handler),
            "returns": self._get_function_return(handler)
        })

    def _get_function_params(self, func: Callable) -> Dict[str, str]:
        """Extract the parameters of a function."""
        params = inspect.signature(func).parameters
        return {name: str(param.annotation) for name, param in params.items()}

    def _get_function_return(self, func: Callable) -> str:
        """Extract the return type of a function."""
        return str(inspect.signature(func).return_annotation)

    def generate_documentation(self) -> str:
        """Generate the API documentation as a JSON string."""
        documentation = {
            "protocols": {}
        }

        for protocol, endpoints in self.endpoints.items():
            documentation["protocols"][protocol] = []
            for endpoint in endpoints:
                documentation["protocols"][protocol].append({
                    "path": endpoint["path"],
                    "auth_method": endpoint["auth_method"],
                    "description": endpoint["description"],
                    "parameters": endpoint["parameters"],
                    "returns": endpoint["returns"]
                })

        return json.dumps(documentation, indent=4)

# Beispiel für Protokoll und Authentifizierung

# Beispielhandler definieren
def example_handler(param1: str, param2: int) -> Dict[str, Any]:
    """Example handler that processes param1 and param2."""
    return {"param1": param1, "param2": param2}

def another_handler(data: Dict[str, Any]) -> str:
    """Another example handler."""
    return "Processed"

# System initialisieren
doc_system = APIDocumentationSystem()

# Endpunkte registrieren
doc_system.register_endpoint(
    protocol="HTTP",
    path="/api/example",
    handler=example_handler,
    auth_method="API Key",
    description="An example endpoint for demonstration."
)

doc_system.register_endpoint(
    protocol="HTTP",
    path="/api/another",
    handler=another_handler,
    auth_method="OAuth2",
    description="Another example endpoint."
)

# Dokumentation generieren und ausgeben
documentation = doc_system.generate_documentation()
print(documentation)
