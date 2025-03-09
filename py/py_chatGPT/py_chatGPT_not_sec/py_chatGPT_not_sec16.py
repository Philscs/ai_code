from flask import Flask, request, jsonify
import threading
import hashlib
import random
import os
import json

# Knoten-Klasse zur Verwaltung von Dateien
class Node:
    def __init__(self, name, address):
        self.name = name
        self.address = address
        self.files = {}

    def store_file(self, file_name, content):
        self.files[file_name] = content
        return f"File {file_name} stored on {self.name}"

    def get_file(self, file_name):
        return self.files.get(file_name, None)

# Load Balancer zur Verteilung von Anfragen
class LoadBalancer:
    def __init__(self, nodes):
        self.nodes = nodes

    def get_node(self, file_name):
        # Hashing zur Bestimmung des Knotens
        hash_value = int(hashlib.md5(file_name.encode()).hexdigest(), 16)
        return self.nodes[hash_value % len(self.nodes)]

# Cluster zur Verwaltung von Knoten und Replikation
class Cluster:
    def __init__(self, replication_factor=2):
        self.nodes = []
        self.load_balancer = None
        self.replication_factor = replication_factor

    def add_node(self, node):
        self.nodes.append(node)
        self.load_balancer = LoadBalancer(self.nodes)

    def store_file(self, file_name, content):
        primary_node = self.load_balancer.get_node(file_name)
        replicas = random.sample(self.nodes, self.replication_factor)
        responses = []

        # Primäre Speicherung
        responses.append(primary_node.store_file(file_name, content))

        # Replikation
        for replica in replicas:
            if replica != primary_node:
                responses.append(replica.store_file(file_name, content))

        return responses

    def retrieve_file(self, file_name):
        primary_node = self.load_balancer.get_node(file_name)
        content = primary_node.get_file(file_name)

        if content:
            return content
        else:
            # Fallback, falls primärer Knoten nicht verfügbar
            for node in self.nodes:
                content = node.get_file(file_name)
                if content:
                    return content

        return None

# Flask-API zur Interaktion mit dem verteilten Dateisystem
app = Flask(__name__)
cluster = Cluster()

@app.route('/add_node', methods=['POST'])
def add_node():
    data = request.json
    node = Node(data['name'], data['address'])
    cluster.add_node(node)
    return jsonify({"message": f"Node {node.name} added."})

@app.route('/store_file', methods=['POST'])
def store_file():
    data = request.json
    responses = cluster.store_file(data['file_name'], data['content'])
    return jsonify({"responses": responses})

@app.route('/retrieve_file/<file_name>', methods=['GET'])
def retrieve_file(file_name):
    content = cluster.retrieve_file(file_name)
    if content:
        return jsonify({"file_name": file_name, "content": content})
    else:
        return jsonify({"error": "File not found."}), 404

if __name__ == '__main__':
    # Beispielknoten erstellen
    cluster.add_node(Node("Node1", "http://localhost:5001"))
    cluster.add_node(Node("Node2", "http://localhost:5002"))
    cluster.add_node(Node("Node3", "http://localhost:5003"))

    # Flask-Server starten
    app.run(port=5000)
