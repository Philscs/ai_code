import inspect

class Container:
    def __init__(self):
        self.services = {}
        self.instances = {}

    def register(self, service, dependencies=None, scope='singleton'):
        if service in self.services:
            raise ValueError(f"Service '{service.__name__}' is already registered.")

        if dependencies is None:
            dependencies = []

        self.services[service] = {
            'dependencies': dependencies,
            'scope': scope
        }

    def resolve(self, service):
        if service not in self.services:
            raise ValueError(f"Service '{service.__name__}' is not registered.")

        if service in self.instances:
            return self.instances[service]

        dependencies = []
        for dependency in self.services[service]['dependencies']:
            if dependency == service:
                raise ValueError(f"Circular dependency detected for service '{service.__name__}'.")

            dependencies.append(self.resolve(dependency))

        instance = service(*dependencies)
        self.instances[service] = instance

        return instance

    def get(self, service):
        if service not in self.services:
            raise ValueError(f"Service '{service.__name__}' is not registered.")

        if service in self.instances:
            return self.instances[service]

        return self.resolve(service)

    def clear(self):
        self.instances = {}


class SingletonScope:
    def __init__(self, container):
        self.container = container

    def __enter__(self):
        pass

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.container.clear()


class PrototypeScope:
    def __init__(self, container):
        self.container = container

    def __enter__(self):
        pass

    def __exit__(self, exc_type, exc_val, exc_tb):
        pass


class ServiceLocator:
    def __init__(self, container):
        self.container = container

    def get(self, service):
        return self.container.get(service)


container = Container()
locator = ServiceLocator(container)

# Usage example
class ServiceA:
    def __init__(self, service_b):
        self.service_b = service_b

class ServiceB:
    def __init__(self, service_a):
        self.service_a = service_a

container.register(ServiceA, dependencies=[ServiceB])
container.register(ServiceB, dependencies=[ServiceA])

with SingletonScope(container):
    service_a = locator.get(ServiceA)
    service_b = locator.get(ServiceB)

print(service_a.service_b)
print(service_b.service_a)
