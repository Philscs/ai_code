from typing import Dict, Any, Set, Optional, Type, TypeVar
from enum import Enum, auto
from dataclasses import dataclass
import inspect
from contextlib import contextmanager
import threading
import weakref

class Scope(Enum):
    SINGLETON = auto()
    TRANSIENT = auto()
    SCOPED = auto()

class LifecycleState(Enum):
    CREATED = auto()
    INITIALIZED = auto()
    DESTROYED = auto()

@dataclass
class ServiceDescriptor:
    service_type: Type
    implementation_type: Type
    scope: Scope
    factory: Optional[callable] = None

class CircularDependencyError(Exception):
    pass

class ServiceNotFoundError(Exception):
    pass

class PrototypePollutionError(Exception):
    pass

T = TypeVar('T')

class DependencyInjector:
    def __init__(self):
        self._services: Dict[Type, ServiceDescriptor] = {}
        self._instances: Dict[Type, Any] = {}
        self._scoped_instances: Dict[int, Dict[Type, Any]] = {}
        self._lifecycle_hooks: Dict[Type, Dict[str, callable]] = {}
        self._current_scope_id = threading.local()
        self._resolution_stack: Set[Type] = set()
        
    def register(self, service_type: Type[T], 
                implementation_type: Optional[Type] = None, 
                scope: Scope = Scope.SINGLETON,
                factory: Optional[callable] = None) -> None:
        """Registriert einen Service mit dem spezifizierten Scope."""
        if implementation_type is None:
            implementation_type = service_type
            
        # Verhindere Prototype Pollution
        if not isinstance(service_type, type) or not isinstance(implementation_type, type):
            raise PrototypePollutionError("Service und Implementation müssen valide Typen sein")
            
        self._services[service_type] = ServiceDescriptor(
            service_type=service_type,
            implementation_type=implementation_type,
            scope=scope,
            factory=factory
        )

    def register_lifecycle_hook(self, service_type: Type,
                              hook_type: str,
                              hook: callable) -> None:
        """Registriert Lifecycle-Hooks für einen Service."""
        if service_type not in self._lifecycle_hooks:
            self._lifecycle_hooks[service_type] = {}
        self._lifecycle_hooks[service_type][hook_type] = hook

    @contextmanager
    def scoped(self):
        """Context Manager für Scope-Isolation."""
        scope_id = id(threading.current_thread())
        self._current_scope_id.id = scope_id
        self._scoped_instances[scope_id] = {}
        try:
            yield
        finally:
            # Cleanup scoped instances
            for instance in self._scoped_instances[scope_id].values():
                self._cleanup_instance(instance)
            del self._scoped_instances[scope_id]
            del self._current_scope_id.id

    def resolve(self, service_type: Type[T]) -> T:
        """Löst eine Dependency auf und handhabt dabei zirkuläre Abhängigkeiten."""
        if service_type in self._resolution_stack:
            raise CircularDependencyError(f"Zirkuläre Abhängigkeit gefunden für {service_type}")
        
        if service_type not in self._services:
            raise ServiceNotFoundError(f"Service nicht registriert: {service_type}")
            
        descriptor = self._services[service_type]
        
        # Prüfe existierende Instanzen basierend auf Scope
        if descriptor.scope == Scope.SINGLETON and service_type in self._instances:
            return self._instances[service_type]
        
        if descriptor.scope == Scope.SCOPED:
            scope_id = getattr(self._current_scope_id, 'id', None)
            if scope_id is None:
                raise RuntimeError("Scoped service kann nur innerhalb eines Scopes aufgelöst werden")
            if service_type in self._scoped_instances[scope_id]:
                return self._scoped_instances[scope_id][service_type]
        
        # Erstelle neue Instanz
        self._resolution_stack.add(service_type)
        try:
            instance = self._create_instance(descriptor)
        finally:
            self._resolution_stack.remove(service_type)
            
        # Speichere Instanz entsprechend ihres Scopes
        if descriptor.scope == Scope.SINGLETON:
            self._instances[service_type] = instance
        elif descriptor.scope == Scope.SCOPED:
            self._scoped_instances[self._current_scope_id.id][service_type] = instance
            
        return instance

    def _create_instance(self, descriptor: ServiceDescriptor) -> Any:
        """Erstellt eine neue Instanz eines Services."""
        if descriptor.factory:
            instance = descriptor.factory()
        else:
            # Analysiere Constructor-Parameter
            sig = inspect.signature(descriptor.implementation_type.__init__)
            params = {}
            
            for param_name, param in sig.parameters.items():
                if param_name == 'self':
                    continue
                    
                if param.annotation == inspect.Parameter.empty:
                    raise ValueError(f"Parameter {param_name} hat keine Type-Annotation")
                    
                params[param_name] = self.resolve(param.annotation)
            
            instance = descriptor.implementation_type(**params)
        
        # Rufe Lifecycle-Hooks auf
        self._initialize_instance(instance, descriptor.implementation_type)
        return instance

    def _initialize_instance(self, instance: Any, service_type: Type) -> None:
        """Initialisiert eine Service-Instanz und ruft entsprechende Hooks auf."""
        if service_type in self._lifecycle_hooks:
            hooks = self._lifecycle_hooks[service_type]
            if 'init' in hooks:
                hooks['init'](instance)

    def _cleanup_instance(self, instance: Any) -> None:
        """Räumt eine Service-Instanz auf und ruft Cleanup-Hooks auf."""
        service_type = type(instance)
        if service_type in self._lifecycle_hooks:
            hooks = self._lifecycle_hooks[service_type]
            if 'cleanup' in hooks:
                hooks['cleanup'](instance)

# Beispiel-Verwendung:
if __name__ == "__main__":
    # Service-Definitionen
    class IDatabase:
        pass

    class ILogger:
        pass

    class Database(IDatabase):
        def __init__(self, logger: ILogger):
            self.logger = logger

    class Logger(ILogger):
        def __init__(self):
            pass

    # Container Setup
    container = DependencyInjector()
    
    # Service-Registrierung
    container.register(ILogger, Logger, Scope.SINGLETON)
    container.register(IDatabase, Database, Scope.SCOPED)
    
    # Lifecycle-Hooks
    container.register_lifecycle_hook(
        Database,
        'init',
        lambda instance: print(f"Database initialized: {instance}")
    )
    
    # Verwendung in einem Scope
    with container.scoped():
        db = container.resolve(IDatabase)
        # Verwendung der Instanz...