import random
from typing import Dict, List, Optional
from datetime import datetime, timedelta
import requests
from dataclasses import dataclass
import threading
import time

@dataclass
class ServiceInstance:
    id: str
    host: str
    port: int
    health: float = 1.0
    last_check: datetime = datetime.now()
    failed_attempts: int = 0

class ServiceRegistry:
    def __init__(self):
        self.services: Dict[str, List[ServiceInstance]] = {}
        self._lock = threading.Lock()
        
    def register(self, service_name: str, instance: ServiceInstance):
        with self._lock:
            if service_name not in self.services:
                self.services[service_name] = []
            self.services[service_name].append(instance)
            
    def unregister(self, service_name: str, instance_id: str):
        with self._lock:
            if service_name in self.services:
                self.services[service_name] = [
                    inst for inst in self.services[service_name] 
                    if inst.id != instance_id
                ]

    def get_instances(self, service_name: str) -> List[ServiceInstance]:
        return self.services.get(service_name, [])

class LoadBalancer:
    def __init__(self, registry: ServiceRegistry):
        self.registry = registry
        
    def get_instance(self, service_name: str) -> Optional[ServiceInstance]:
        instances = self.registry.get_instances(service_name)
        healthy_instances = [
            inst for inst in instances 
            if inst.health > 0.5 and inst.failed_attempts < 3
        ]
        
        if not healthy_instances:
            return None
            
        # Gewichtete Zufallsauswahl basierend auf Gesundheitsstatus
        weights = [inst.health for inst in healthy_instances]
        return random.choices(healthy_instances, weights=weights, k=1)[0]

class CircuitBreaker:
    def __init__(self, failure_threshold: int = 3, reset_timeout: int = 60):
        self.failure_threshold = failure_threshold
        self.reset_timeout = reset_timeout
        self.failure_count: Dict[str, int] = {}
        self.last_failure_time: Dict[str, datetime] = {}
        self._lock = threading.Lock()
        
    def can_execute(self, service_instance: ServiceInstance) -> bool:
        with self._lock:
            instance_key = f"{service_instance.host}:{service_instance.port}"
            
            # Prüfen ob Circuit Breaker zurückgesetzt werden soll
            if instance_key in self.last_failure_time:
                last_failure = self.last_failure_time[instance_key]
                if datetime.now() - last_failure > timedelta(seconds=self.reset_timeout):
                    self.failure_count[instance_key] = 0
                    del self.last_failure_time[instance_key]
                    
            return self.failure_count.get(instance_key, 0) < self.failure_threshold
            
    def record_failure(self, service_instance: ServiceInstance):
        with self._lock:
            instance_key = f"{service_instance.host}:{service_instance.port}"
            self.failure_count[instance_key] = self.failure_count.get(instance_key, 0) + 1
            self.last_failure_time[instance_key] = datetime.now()
            
    def record_success(self, service_instance: ServiceInstance):
        with self._lock:
            instance_key = f"{service_instance.host}:{service_instance.port}"
            self.failure_count[instance_key] = 0
            if instance_key in self.last_failure_time:
                del self.last_failure_time[instance_key]

class MicroserviceManager:
    def __init__(self):
        self.registry = ServiceRegistry()
        self.load_balancer = LoadBalancer(self.registry)
        self.circuit_breaker = CircuitBreaker()
        
    def call_service(self, service_name: str, endpoint: str, method: str = 'GET', **kwargs) -> Optional[requests.Response]:
        instance = self.load_balancer.get_instance(service_name)
        if not instance:
            raise Exception(f"Kein verfügbarer Service: {service_name}")
            
        if not self.circuit_breaker.can_execute(instance):
            raise Exception(f"Circuit Breaker offen für: {instance.host}:{instance.port}")
            
        try:
            url = f"http://{instance.host}:{instance.port}/{endpoint.lstrip('/')}"
            response = requests.request(method, url, **kwargs)
            
            if response.status_code >= 500:
                self.circuit_breaker.record_failure(instance)
                instance.failed_attempts += 1
                instance.health *= 0.8  # Reduziere Gesundheitsstatus
            else:
                self.circuit_breaker.record_success(instance)
                instance.failed_attempts = 0
                instance.health = min(1.0, instance.health + 0.1)  # Verbessere Gesundheitsstatus
                
            return response
            
        except requests.RequestException as e:
            self.circuit_breaker.record_failure(instance)
            instance.failed_attempts += 1
            instance.health *= 0.5  # Stark reduzierter Gesundheitsstatus bei Verbindungsfehlern
            raise

# Beispiel zur Verwendung:
if __name__ == "__main__":
    # Manager erstellen
    manager = MicroserviceManager()
    
    # Services registrieren
    service1 = ServiceInstance("service1-1", "localhost", 8001)
    service2 = ServiceInstance("service1-2", "localhost", 8002)
    manager.registry.register("service1", service1)
    manager.registry.register("service1", service2)
    
    # Service aufrufen
    try:
        response = manager.call_service("service1", "/api/data")
        print(f"Response: {response.status_code}")
    except Exception as e:
        print(f"Error: {e}")