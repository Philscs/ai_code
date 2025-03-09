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
        """Generiert OpenAPI-Spezifikation im JSON-Format"""
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
                        "in": "query",  # Vereinfachte Annahme
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
        """Generiert Markdown-Dokumentation"""
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
    """Dekorator für die automatische Dokumentation von API-Endpunkten"""
    def decorator(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            return func(*args, **kwargs)
        
        # Speichere Dokumentationsinformationen in der Funktion
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

# Beispiel für die Verwendung:
if __name__ == "__main__":
    # API-Dokumentation erstellen
    docs = APIDocumentation(
        title="Beispiel API",
        version="1.0.0",
        protocol=Protocol.REST
    )
    docs.set_base_url("https://api.example.com/v1")
    
    # Beispiel-Endpunkt definieren
    endpoint = Endpoint(
        path="/users",
        method="GET",
        description="Liste aller Benutzer abrufen",
        parameters=[
            EndpointParameter(
                name="page",
                type="integer",
                required=False,
                description="Seitennummer für Pagination",
                default=1
            )
        ],
        responses=[
            EndpointResponse(
                status_code=200,
                description="Erfolgreiche Anfrage",
                schema={
                    "type": "array",
                    "items": {
                        "type": "object",
                        "properties": {
                            "id": {"type": "integer"},
                            "name": {"type": "string"},
                            "email": {"type": "string"}
                        }
                    }
                }
            )
        ],
        auth_required=True,
        auth_types=[AuthType.BEARER]
    )
    
    docs.add_endpoint(endpoint)
    
    # Dokumentation generieren
    openapi_spec = docs.generate_openapi_spec()
    markdown_docs = docs.generate_markdown()
    
    # Speichern der Dokumentation
    with open("api_docs.json", "w") as f:
        json.dump(openapi_spec, f, indent=2)
        
    with open("api_docs.md", "w") as f:
        f.write(markdown_docs)