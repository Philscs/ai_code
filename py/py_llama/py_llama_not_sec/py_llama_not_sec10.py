import os
from flask import Flask, request, jsonify
from flask_socketio import SocketIO, emit
import socketio
from cryptography.fernet import Fernet
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from flask_sqlalchemy import SQLAlchemy
import base64

class Server:
    def __init__(self, server_port=5000):
        self.server_port = server_port
        self.rooms = {}
        self.users = {}

    def start(self):
        sio = socketio.SIO()
        app = Flask(__name__)
        sio.server.register_namespace(Namespace)
        return sio.run(app)

class Client:
    def __init__(self, client_port=5001, namespace="/"):
        self.client_port = client_port
        self.namespace = namespace
        self.sio = SocketIO(self.client_port, namespace=self.namespace)
        self.sio.connect("http://localhost:5000")

    def send_message(self, message):
        self.sio.emit("message", message)

class Room:
    def __init__(self, room_id, name):
        self.room_id = room_id
        self.name = name
        self.users = []

class User:
    def __init__(self, user_id, username, password):
        self.user_id = user_id
        self.username = username
        self.password = password

def generate_key(password):
    kdf = PBKDF2HMAC(
        algorithm=hashes.SHA256(),
        length=32,
        salt=b"salt",
        iterations=100000,
    )
    key = base64.urlsafe_b64encode(kdf.derive(password.encode()))
    return key

def encrypt(message, key):
    cipher_suite = Fernet(key)
    encrypted_message = cipher_suite.encrypt(message.encode())
    return encrypted_message.decode()

def decrypt(encrypted_message, key):
    cipher_suite = Fernet(key)
    decrypted_message = cipher_suite.decrypt(encrypted_message.encode())
    return decrypted_message.decode()

class Namespace:
    def __init__(self):
        self.server = Server()
        self.client = Client()

    def on_connect(self):
        print("Client connected")

    def on_disconnect(self):
        print("Client disconnected")

def main():
    namespace = Namespace()
    sio = socketio.Client(namespace=namespace)
    sio.on('connect', namespace.on_connect)
    sio.on('disconnect', namespace.on_disconnect)

    # Erstellen Sie einen neuen Raum
    room = Room(1, "Leer")
    user = User(2, "Testnutzer", "123456")

    # Fügen Sie den Benutzer zum Raum hinzu
    room.users.append(user)

    sio.connect("http://localhost:5000")

    while True:
        message = input()
        namespace.client.send_message(message)

if __name__ == "__main__":
    main()