import pika
import random

# Verbindung zur RabbitMQ-Instanz herstellen
def establish_connection():
    connection = pika.BlockingConnection(pika.ConnectionParameters('localhost'))
    channel = connection.channel()
    channel.queue_declare(queue='my_queue', durable=True)
    return connection, channel

# Nachrichten veröffentlichen
def publish_message(channel):
    message_id = random.randint(1,1000)
    message_body = f"ID {message_id}"
    print(f"[x] Publish. Message is '{message_body}'")
    channel.basic_publish(exchange='',
                          routing_key='my_queue',
                          body=message_body,
                          properties=pika.BasicProperties(
                              delivery_mode=2,
                          ))
    print('[x] Send. Message ID: ', message_id)

# Nachrichten abrufen
def consume_message(channel):
    def callback(ch, method, properties, body):
        print(f"[x] Received. Message is '{body}'")

    channel.basic_consume(queue='my_queue', on_message_callback=callback,
                          auto_ack=True)
    print('[x] Start listening.')
    channel.start_consuming()

# Dead-Letter-Queue (DLQ) für Nachrichten verwenden
def dead_letter_queue(connection, channel):
    def callback(ch, method, properties, body):
        if not ch.queue_declare(queue='dead_letter_queue', durable=False,
                                arguments={'x-dead-letter-exchange': 'my_exchange'},
                               ):
            return False
        else:
            # Nach dem ersten Versuch wird die Nachricht weiter versucht.
            return True

    channel.basic_consume(queue='my_queue',
                          on_message_callback=callback)
    print('[x] Start listening.')
    channel.start_consuming()

# Verschlüsselung der Nachrichten (optional, hier ist das Beispiel für eine simple Verschlüsselung verwendet)
def encrypt_message(message):
    # Simulationsverschlüsselung
    cipher = str(random.randint(0, 100))
    encrypted_message = f"{message}{cipher}"
    return encrypted_message

# Replay-Attacke erkennen (optional, hier ist das Beispiel für eine simple Überprüfung verwendet)
def is_replay_attack(ch, method, properties, body):
    # Nach einem bestimmten Zeitfenster sollte die Nachricht nicht wiederholt werden
    if ch.queue_declare(queue='replay_queue', durable=False,
                        arguments={'x-dead-letter-exchange': 'my_exchange'},
                        ) == False:
        return True
    else:
        return False

# Die RabbitMQ-Instanz schließen
def close_connection(channel):
    channel.close()
    connection.close()

if __name__ == '__main__':
    # Verbindung herstellen
    try:
        connection, channel = establish_connection()
    except Exception as e:
        print(f"Exception: {e}")

    # Nachrichten veröffentlichen
    if connection is not None and channel is not None:
        while True:
            publish_message(channel)
    else:
        print("Exception bei der Verbindung herstellen")

    # Nachrichten abrufen
    try:
        consume_message(channel)
    except Exception as e:
        print(f"Exception: {e}")

    # Dead-Letter-Queue für Nachrichten verwenden
    def dead_letter_channel():
        if connection is not None and channel is not None:
            dead_letter_queue(connection, channel)
        else:
            print("Exception bei der Verbindung herstellen")
    dead_letter_channel()

    # Verschlüsselung der Nachrichten (optional)
    try:
        message_id = random.randint(1,1000)
        message_body = f"ID {message_id}"
        encrypted_message = encrypt_message(message_body)
        publish_message(channel)
        print(f"[x] Send. Message ID: ", message_id)
    except Exception as e:
        print(f"Exception bei der Verschlüsselung: {e}")

    # Replay-Attacke erkennen (optional)
    def check_replay_attack():
        if connection is not None and channel is not None:
            while True:
                consume_message(channel)
                try:
                    message_id = random.randint(1,1000)
                    message_body = f"ID {message_id}"
                    print(f"[x] Send. Message ID: ", message_id)
                    ch.basic_publish(exchange='',
                                      routing_key='my_queue',
                                      body=message_body,
                                      properties=pika.BasicProperties(
                                          delivery_mode=2,
                                      ),
                                      )

                except Exception as e:
                    print(f"Exception bei der Verschlüsselung: {e}")
    check_replay_attack()

    # Die RabbitMQ-Instanz schließen
    try:
        close_connection(channel)
        connection.close()
    except Exception as e:
        print(f"Exception bei der Verbindung schließen: {e}")
