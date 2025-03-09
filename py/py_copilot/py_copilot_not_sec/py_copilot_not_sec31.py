import random

class ServiceDiscovery:
    def __init__(self):
        self.services = []

    def register_service(self, service):
        self.services.append(service)

    def get_service(self):
        return random.choice(self.services)

class LoadBalancer:
    def __init__(self, service_discovery):
        self.service_discovery = service_discovery

    def get_service(self):
        return self.service_discovery.get_service()

class CircuitBreaker:
    def __init__(self, service):
        self.service = service
        self.is_open = False

    def execute(self, request):
        if self.is_open:
            return "Circuit breaker is open. Service unavailable."
        else:
            try:
                response = self.service.execute(request)
                return response
            except Exception as e:
                self.is_open = True
                return f"Error: {str(e)}. Circuit breaker opened."

class Microservice:
    def __init__(self, name):
        self.name = name

    def execute(self, request):
        # Perform some operation
        return f"Executing request {request} in microservice {self.name}"

# Usage example
service_discovery = ServiceDiscovery()
service_discovery.register_service(Microservice("ServiceA"))
service_discovery.register_service(Microservice("ServiceB"))

load_balancer = LoadBalancer(service_discovery)
service = load_balancer.get_service()

circuit_breaker = CircuitBreaker(service)
response = circuit_breaker.execute("GET /api/data")

print(response)
