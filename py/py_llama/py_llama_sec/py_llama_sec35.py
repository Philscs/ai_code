import importlib
from abc import ABC, abstractmethod
from typing import Callable, Dict

class Service(ABC):
    @abstractmethod
    def do_something(self) -> None:
        pass

class DependencyInjector:
    def __init__(self):
        self.services: Dict[str, 'Service'] = {}

    def register_service(self, service_name: str, service_class: type, scope: str = 'request') -> None:
        if scope not in ['request', 'singleton']:
            raise ValueError(f"Invalid scope '{scope}'. Only 'request' and 'singleton' are 
supported.")
        
        if service_name in self.services:
            raise ValueError(f"A service with the same name '{service_name}' is already 
registered.")

        self.services[service_name] = {
            'class': service_class,
            'scope': scope
        }

    def get_service(self, service_name: str) -> Service:
        return self.services[service_name]['class']()

    def inject_dependencies(self, service: Service, dependencies: Dict[str, Service]) -> None:
        for dependency_name, dependency in dependencies.items():
            if dependency_name not in self.services:
                raise ValueError(f"Dependency '{dependency_name}' is not registered.")

            setattr(service, dependency_name, self.get_service(dependency_name))

class ServiceLocator:
    def __init__(self):
        self.injector = DependencyInjector()

    def register(self, service_name: str, service_class: type) -> None:
        return self.injector.register_service(
            service_name=service_name,
            service_class=service_class
        )

    def get(self, service_name: str) -> Service:
        return self.injector.get_service(service_name)

class PrototypeService(ABC):
    @abstractmethod
    def create_prototype(self) -> 'PrototypeService':
        pass

class PrototypeDependencyInjector(DependencyInjector):
    def __init__(self):
        super().__init__()
        self.prototypes: Dict[str, PrototypeService] = {}

    def register_service(self, service_name: str, service_class: type) -> None:
        if service_name in self.services:
            raise ValueError(f"A service with the same name '{service_name}' is already 
registered.")

        self.services[service_name] = {
            'class': service_class
        }

    def get_prototype(self, service_name: str) -> PrototypeService:
        return self.prototypes.setdefault(service_name, self.services[service_name]['class']())

def create_prototype(service: PrototypeService) -> PrototypeService:
    if not hasattr(service, '_prototype'):
        setattr(service, '_prototype', service.create_prototype())
    return service._prototype

if __name__ == "__main__":
    class Service1(Service):
        def do_something(self) -> None:
            print(f"Service 1 is doing something.")

    class Service2(Service):
        def do_something_else(self) -> None:
            print(f"Service 2 is doing something else.")

    service_locator = ServiceLocator()

    service_locator.register("service1", Service1)
    service_locator.register("service2", Service2)

    # Register prototype
    class PrototypeService1(PrototypeService):
        def create_prototype(self) -> 'PrototypeService':
            return self

    service_locator.register("prototype_service1", PrototypeService1)
    PrototypeDependencyInjector().register_service(
        "prototype_service1",
        PrototypeService1
    )

    service1 = service_locator.get("service1")
    service2 = service_locator.get("service2")

    # Inject dependencies
    class MyService(Service):
        def __init__(self, service1: Service) -> None:
            self.service1 = service1

        def do_something(self) -> None:
            self.service1.do_something()

    PrototypeDependencyInjector().register_service(
        "my_service",
        MyService
    )
    PrototypeDependencyInjector().inject_dependencies(service2, {"service1": 
service_locator.get("service1")})

    # Use prototype
    service3 = PrototypeDependencyInjector().get_prototype("prototype_service1")
    create_prototype(service3).do_something()