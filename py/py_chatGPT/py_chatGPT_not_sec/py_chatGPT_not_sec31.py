from flask import Flask, request, jsonify
import requests
import random
import time

class ServiceRegistry:
    def __init__(self):
        self.services = {}

    def register(self, service_name, service_url):
        if service_name not in self.services:
            self.services[service_name] = []
        if service_url not in self.services[service_name]:
            self.services[service_name].append(service_url)

    def deregister(self, service_name, service_url):
        if service_name in self.services and service_url in self.services[service_name]:
            self.services[service_name].remove(service_url)

    def get_services(self, service_name):
        return self.services.get(service_name, [])

class LoadBalancer:
    @staticmethod
    def select_service(services):
        return random.choice(services) if services else None

class CircuitBreaker:
    def __init__(self, failure_threshold=3, recovery_time=10):
        self.failure_counts = {}
        self.last_failed_time = {}
        self.failure_threshold = failure_threshold
        self.recovery_time = recovery_time

    def can_call(self, service_url):
        if service_url not in self.failure_counts:
            return True

        if self.failure_counts[service_url] < self.failure_threshold:
            return True

        time_since_last_failure = time.time() - self.last_failed_time[service_url]
        if time_since_last_failure >= self.recovery_time:
            self.reset(service_url)
            return True

        return False

    def record_failure(self, service_url):
        if service_url not in self.failure_counts:
            self.failure_counts[service_url] = 0
        self.failure_counts[service_url] += 1
        self.last_failed_time[service_url] = time.time()

    def reset(self, service_url):
        self.failure_counts[service_url] = 0
        self.last_failed_time.pop(service_url, None)

app = Flask(__name__)
registry = ServiceRegistry()
balancer = LoadBalancer()
circuit_breaker = CircuitBreaker()

@app.route('/register', methods=['POST'])
def register_service():
    data = request.json
    service_name = data['service_name']
    service_url = data['service_url']
    registry.register(service_name, service_url)
    return jsonify({'message': f'Service {service_name} at {service_url} registered successfully.'})

@app.route('/deregister', methods=['POST'])
def deregister_service():
    data = request.json
    service_name = data['service_name']
    service_url = data['service_url']
    registry.deregister(service_name, service_url)
    return jsonify({'message': f'Service {service_name} at {service_url} deregistered successfully.'})

@app.route('/invoke/<service_name>', methods=['GET'])
def invoke_service(service_name):
    services = registry.get_services(service_name)
    service_url = balancer.select_service(services)

    if not service_url:
        return jsonify({'error': 'No services available.'}), 503

    if not circuit_breaker.can_call(service_url):
        return jsonify({'error': 'Circuit breaker open.'}), 503

    try:
        response = requests.get(service_url)
        response.raise_for_status()
        return jsonify({'data': response.json()})
    except requests.RequestException:
        circuit_breaker.record_failure(service_url)
        return jsonify({'error': 'Service call failed.'}), 500

if __name__ == '__main__':
    app.run(debug=True, port=5000)
