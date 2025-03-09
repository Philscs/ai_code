import time
import random

class ServiceDiscovery:
    def __init__(self):
        self.services = {}

    def register_service(self, service_name, ip_address, port):
        self.services[service_name] = (ip_address, port)

    def get_service(self, service_name):
        return self.services.get(service_name)


class LoadBalancer:
    def __init__(self):
        self.services = []

    def add_service(self, service_name, ip_address, port):
        self.services.append((service_name, ip_address, port))

    def get_next_service(self, service_name):
        return self.services[(self.services.index((service_name, *args)) + 1) % 
len(self.services)]


class CircuitBreaker:
    def __init__(self):
        self.services = {}
        self.fails = {}

    def register_service(self, service_name):
        self.services[service_name] = 0
        self.fails[service_name] = []

    def fault(self, service_name):
        if service_name in self.fails:
            self.fails[service_name].append(time.time())
        else:
            self.services[service_name] += 1

    def reset_service(self, service_name):
        if service_name in self.fails:
            del self.fails[service_name]

    def is_closed(self, service_name):
        return self.services[service_name] < 3


class OrderService:
    def __init__(self, service_discovery, circuit_breaker):
        self.service_discovery = service_discovery
        self.circuit_breaker = circuit_breaker

    def process_order(self, order_id):
        if not self.circuit_breaker.is_closed('order-service'):
            raise Exception("Circuit Broken")

        service_name = 'order-service'
        ip_address, port = self.service_discovery.get_service(service_name)
        client = Client(ip_address, port)

        response = client.process_order(order_id)
        print(response)


class Client:
    def __init__(self, ip_address, port):
        self.ip_address = ip_address
        self.port = port

    def process_order(self, order_id):
        time.sleep(random.uniform(0.1, 0.5))  # Simulate Some IO Time
        if random.random() < 0.2:
            raise Exception("Fehler beim Prozessieren von Order")
        return f"Order {order_id} processed"


def main():
    service_discovery = ServiceDiscovery()
    load_balancer = LoadBalancer()
    circuit_breaker = CircuitBreaker()

    # Registrierung der Dienste
    service_discovery.register_service('order-service', '127.0.0.1', 8080)
    load_balancer.add_service('order-service', '127.0.0.1', 8080)

    for i in range(10):
        order_service = OrderService(service_discovery, circuit_breaker)
        try:
            order_service.process_order(f"Order {i}")
        except Exception as e:
            print(e)

        time.sleep(2)  # Simulate Some IO Time


if __name__ == "__main__":
    main()