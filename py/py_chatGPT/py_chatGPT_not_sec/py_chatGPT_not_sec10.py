import asyncio
import pickle
from cryptography.fernet import Fernet

# Schlüssel für die Verschlüsselung erzeugen
def generate_key():
    return Fernet.generate_key()

class ChatRoom:
    def __init__(self, name):
        self.name = name
        self.participants = []

    async def broadcast(self, message, sender=None):
        for participant in self.participants:
            if participant != sender:
                await participant.send_message(message)

class User:
    def __init__(self, username, writer, key):
        self.username = username
        self.writer = writer
        self.key = key
        self.cipher = Fernet(key)

    async def send_message(self, message):
        encrypted_message = self.cipher.encrypt(message.encode())
        self.writer.write(encrypted_message + b"\n")
        await self.writer.drain()

    def decrypt_message(self, message):
        return self.cipher.decrypt(message).decode()

class ChatServer:
    def __init__(self):
        self.rooms = {}
        self.users = {}
        self.offline_messages = {}

    def get_or_create_room(self, room_name):
        if room_name not in self.rooms:
            self.rooms[room_name] = ChatRoom(room_name)
        return self.rooms[room_name]

    async def handle_client(self, reader, writer):
        username = await reader.readuntil(b"\n")
        username = username.decode().strip()
        key = generate_key()

        user = User(username, writer, key)
        self.users[username] = user
        self.offline_messages.setdefault(username, [])

        await self.send_offline_messages(user)

        while True:
            try:
                data = await reader.readuntil(b"\n")
                if not data:
                    break
                message = user.decrypt_message(data.strip())
                await self.process_message(message, user)
            except asyncio.IncompleteReadError:
                break

        del self.users[username]

    async def process_message(self, message, sender):
        if message.startswith("/room"):
            _, room_name = message.split(" ", 1)
            room = self.get_or_create_room(room_name)
            room.participants.append(sender)
            await room.broadcast(f"{sender.username} hat den Raum betreten.", sender)

        elif message.startswith("/msg"):
            _, recipient, private_message = message.split(" ", 2)
            if recipient in self.users:
                user = self.users[recipient]
                await user.send_message(f"{sender.username} (privat): {private_message}")
            else:
                self.offline_messages[recipient].append((sender.username, private_message))

        elif message.startswith("/file"):
            _, recipient, filename = message.split(" ", 2)
            if recipient in self.users:
                user = self.users[recipient]
                await self.send_file(user, filename)
            else:
                self.offline_messages[recipient].append((sender.username, f"Datei: {filename}"))

        elif message.startswith("/leave"):
            _, room_name = message.split(" ", 1)
            room = self.rooms.get(room_name)
            if room:
                room.participants.remove(sender)
                await room.broadcast(f"{sender.username} hat den Raum verlassen.", sender)

        else:
            await sender.send_message("Unbekannter Befehl.")

    async def send_file(self, recipient, filename):
        try:
            with open(filename, "rb") as file:
                data = pickle.dumps(file.read())
                encrypted_data = recipient.cipher.encrypt(data)
                await recipient.send_message(f"Datei {filename} gesendet: {encrypted_data.hex()}")
        except FileNotFoundError:
            await recipient.send_message("Datei nicht gefunden.")

    async def send_offline_messages(self, user):
        messages = self.offline_messages[user.username]
        for sender, msg in messages:
            await user.send_message(f"Offline Nachricht von {sender}: {msg}")
        self.offline_messages[user.username] = []

    async def start(self, host="127.0.0.1", port=8888):
        server = await asyncio.start_server(self.handle_client, host, port)
        print(f"Server gestartet auf {host}:{port}")

        async with server:
            await server.serve_forever()

if __name__ == "__main__":
    server = ChatServer()
    asyncio.run(server.start())
