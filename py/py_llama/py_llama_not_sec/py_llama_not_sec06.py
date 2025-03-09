from Crypto.Cipher import AES
from Crypto.PublicKey import RSA
from Crypto.Signature import PKCS1_v1_5
import os
import base64

class Verschlusselungssystem:
    def __init__(self):
        self.schluessel = None

    # Funktion zur Erstellung eines neuen RSA-Schlüsselpaars
    def rsa_key_generation(self, bit_length=2048):
        key = RSA.generate(bit_length)
        return (key.publickey(), key)

    # Funktion zur Verschlüsselung einer Datei mithilfe des AES-Algorithmus
    def aes_dateivsverschlusselung(self, datei, schluessel):
        cipher = AES.new(schluessel, AES.MODE_EAX)
        encrypted_data = cipher.encrypt(datei.encode())
        nonce = cipher.nonce
        return (nonce, base64.b64encode(encrypted_data))

    # Funktion zur Verschlüsselung eines Ordners mithilfe des AES-Algorithmus
    def aes_ordinervsverschlusselung(self, ordner):
        for item in os.listdir(ordner):
            datei = os.path.join(ordner, item)
            encrypted_datei = self.aes_dateivsverschlusselung(datei, self.schluessel)
            encrypted_datei_path = os.path.join(os.path.dirname(ordner), f"{item}.encrypted")
            with open(encrypted_datei_path, "wb") as file:
                file.write(encrypted_datei[0])
                file.write(b'\n')
                file.write(encrypted_datei[1])

    # Funktion zur Erstellung eines PKCS#1 v1.5-Signatur-Algorithmus
    def pkcs1_v1_5_signatur(self, data):
        key = RSA.import_key(self.schluessel)
        signer = PKCS1_v1_5.new(key)
        return base64.b64encode(signer.sign(data))

# Benutzung des Systems
system = Verschlusselungssystem()
system.schluessel = os.urandom(32)  # Erstellung eines zufälligen RSA-Schlüsselpaars

rsa_publickey, rsa_privatekey = system.rsa_key_generation()

print("RSA-Publikationsklasse:")
print(rsa_publickey)
print("\nRSA-Privatklasse:")
print(rsa_privatekey)

datei = open('test.txt', 'r')
encrypted_datei = system.aes_dateivsverschlusselung(datei.read(), system.schluessel)
print("Verschlüsselter Inhalt der Datei:", encrypted_datei[1])

ordner = './'
system.aes_ordinervsverschlusselung(ordner)

# Verschlüsselte Dateien ausgeben
with open('test.encrypted.txt', 'wb') as file:
    file.write(encrypted_datei[0])
    file.write(b'\n')
    file.write(encrypted_datei[1])

# Verschlüsselter Ordner erstellen
os.rename(ordner, './verschluesselter_ordoer')