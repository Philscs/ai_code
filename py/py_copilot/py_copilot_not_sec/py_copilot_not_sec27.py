from typing import Dict, List, Optional, Union
from dataclasses import dataclass
from enum import Enum
import inspect
import json
import yaml
from functools import wraps


class Protocol(Enum):
    REST = "REST"
    GRAPHQL = "GraphQL"
    GRPC = "gRPC"
    SOAP = "SOAP"


class AuthType(Enum):
    NONE = "None"
    API_KEY = "API Key"
    BEARER = "Bearer Token"
    BASIC = "Basic Auth"
    OAUTH2 = "OAuth 2.0"


@dataclass
class EndpointParameter:
    name: str
    type: str
    required: bool
    description: str
    default: Optional[any] = None


@dataclass
class EndpointResponse:
    status_code: int
    description: str
    schema: Dict


@dataclass
class Endpoint:
    path: str
    method: str
    description: str
    parameters: List[EndpointParameter]
    responses: List[EndpointResponse]
    auth_required: bool
    auth_types: List[AuthType]


class APIDocumentation:
    def __init__(self, title: str, version: str, protocol: Protocol):
        self.title = title
        self.version = version
        self.protocol = protocol
        self.endpoints: List[Endpoint] = []
        self.base_url: Optional[str] = None

    def set_base_url(self, url: str):
        self.base_url = url

    def add_endpoint(self, endpoint: Endpoint):
        self.endpoints.append(endpoint)

    def generate_openapi_spec(self) -> Dict:
        """Generates OpenAPI specification in JSON format"""
        spec = {
            "openapi": "3.0.0",
            "info": {
                "title": self.title,
                "version": self.version
            },
            "servers": [{"url": self.base_url}] if self.base_url else [],
            "paths": {}
        }

        for endpoint in self.endpoints:
            path_spec = {
                "description": endpoint.description,
                "parameters": [
                    {
                        "name": param.name,
                        "in": "query",  # Simplified assumption
                        "required": param.required,
                        "schema": {"type": param.type},
                        "description": param.description
                    }
                    for param in endpoint.parameters
                ],
                "responses": {
                    str(resp.status_code): {
                        "description": resp.description,
                        "content": {
                            "application/json": {
                                "schema": resp.schema
                            }
                        }
                    }
                    for resp in endpoint.responses
                }
            }

            if endpoint.auth_required:
                path_spec["security"] = [
                    {auth_type.value.lower(): []} for auth_type in endpoint.auth_types
                ]

            if endpoint.path not in spec["paths"]:
                spec["paths"][endpoint.path] = {}
            spec["paths"][endpoint.path][endpoint.method.lower()] = path_spec

        return spec

    def generate_markdown(self) -> str:
        """Generates Markdown documentation"""
        md = f"# {self.title}\n\n"
        md += f"Version: {self.version}\n"
        md += f"Protocol: {self.protocol.value}\n\n"

        if self.base_url:
            md += f"Base URL: {self.base_url}\n\n"

        for endpoint in self.endpoints:
            md += f"## {endpoint.method} {endpoint.path}\n\n"
            md += f"{endpoint.description}\n\n"

            if endpoint.auth_required:
                md += "### Authentication\n"
                md += "Required authentication types:\n"
                for auth_type in endpoint.auth_types:
                    md += f"- {auth_type.value}\n"
                md += "\n"

            if endpoint.parameters:
                md += "### Parameters\n\n"
                md += "| Name | Type | Required | Description | Default |\n"
                md += "|------|------|----------|-------------|---------|\n"
                for param in endpoint.parameters:
                    md += f"| {param.name} | {param.type} | {param.required} | {param.description} | {param.default or '-'} |\n"
                md += "\n"

            if endpoint.responses:
                md += "### Responses\n\n"
                for resp in endpoint.responses:
                    md += f"#### {resp.status_code}\n"
                    md += f"{resp.description}\n\n"
                    md += "```json\n"
                    md += json.dumps(resp.schema, indent=2)
                    md += "\n```\n\n"

        return md


def document_endpoint(
    path: str,
    method: str,
    description: str,
    parameters: List[EndpointParameter] = None,
    responses: List[EndpointResponse] = None,
    auth_required: bool = False,
    auth_types: List[AuthType] = None
):
    """Decorator for automatic documentation of API endpoints"""
    def decorator(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            return func(*args, **kwargs)

        # Store documentation information in the function
        wrapper.__api_docs__ = Endpoint(
            path=path,
            method=method,
            description=description,
            parameters=parameters or [],
            responses=responses or [],
            auth_required=auth_required,
            auth_types=auth_types or []
        )
        return wrapper
    return decorator


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
