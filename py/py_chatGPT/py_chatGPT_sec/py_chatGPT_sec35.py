from typing import Callable, Dict, Type, Any

class CircularDependencyError(Exception):
    pass

class ServiceNotFoundError(Exception):
    pass

class DependencyInjector:
    def __init__(self):
        self._services = {}
        self._singletons = {}
        self._construction_stack = []

    def register(self, name: str, factory: Callable, lifecycle: str = "singleton"):
        if lifecycle not in {"singleton", "transient"}:
            raise ValueError("Lifecycle must be 'singleton' or 'transient'.")
        
        self._services[name] = {
            "factory": factory,
            "lifecycle": lifecycle
        }

    def resolve(self, name: str):
        if name in self._singletons:
            return self._singletons[name]

        if name in self._construction_stack:
            raise CircularDependencyError(f"Circular dependency detected for service: {name}")

        service_config = self._services.get(name)
        if not service_config:
            raise ServiceNotFoundError(f"Service '{name}' not found.")

        self._construction_stack.append(name)
        instance = service_config["factory"](self)
        self._construction_stack.pop()

        if service_config["lifecycle"] == "singleton":
            self._singletons[name] = instance

        return instance

    def create_scope(self):
        return ScopedDependencyInjector(self)

class ScopedDependencyInjector:
    def __init__(self, parent_injector: DependencyInjector):
        self._parent = parent_injector
        self._local_singletons = {}

    def resolve(self, name: str):
        if name in self._local_singletons:
            return self._local_singletons[name]

        service_config = self._parent._services.get(name)
        if not service_config:
            raise ServiceNotFoundError(f"Service '{name}' not found in scope.")

        if name in self._parent._construction_stack:
            raise CircularDependencyError(f"Circular dependency detected for service: {name}")

        self._parent._construction_stack.append(name)
        instance = service_config["factory"](self)
        self._parent._construction_stack.pop()

        if service_config["lifecycle"] == "singleton":
            self._local_singletons[name] = instance

        return instance

# Beispiel-Nutzung
def service_a_factory(di):
    service_b = di.resolve("service_b")
    return {"name": "Service A", "dependency": service_b}

def service_b_factory(di):
    return {"name": "Service B"}

di = DependencyInjector()
di.register("service_a", service_a_factory, lifecycle="singleton")
di.register("service_b", service_b_factory, lifecycle="singleton")

# Auflösen von Diensten
try:
    service_a = di.resolve("service_a")
    print(service_a)
except CircularDependencyError as e:
    print(f"Fehler: {e}")
