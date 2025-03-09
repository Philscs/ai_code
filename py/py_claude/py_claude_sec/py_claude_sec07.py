import crypto from 'crypto';
import fs from 'fs/promises';

interface Message {
    id: string;
    topic: string;
    content: string;
    timestamp: number;
    nonce: string;
}

interface EncryptedMessage {
    id: string;
    topic: string;
    encryptedContent: string;
    iv: string;
    timestamp: number;
    nonce: string;
}

class MessageBroker {
    private subscribers: Map<string, Set<(message: Message) => void>>;
    private messageStore: Map<string, EncryptedMessage>;
    private deadLetterQueue: EncryptedMessage[];
    private processingHistory: Set<string>;
    private readonly secretKey: Buffer;
    private readonly persistencePath: string;
    
    constructor(secretKey: string, persistencePath: string) {
        this.subscribers = new Map();
        this.messageStore = new Map();
        this.deadLetterQueue = [];
        this.processingHistory = new Set();
        this.secretKey = Buffer.from(secretKey, 'hex');
        this.persistencePath = persistencePath;
    }

    // Nachricht verschlüsseln
    private encryptMessage(message: Message): EncryptedMessage {
        const iv = crypto.randomBytes(16);
        const cipher = crypto.createCipheriv('aes-256-gcm', this.secretKey, iv);
        
        const encryptedContent = Buffer.concat([
            cipher.update(JSON.stringify(message.content), 'utf8'),
            cipher.final()
        ]).toString('base64');

        return {
            id: message.id,
            topic: message.topic,
            encryptedContent,
            iv: iv.toString('hex'),
            timestamp: message.timestamp,
            nonce: message.nonce
        };
    }

    // Nachricht entschlüsseln
    private decryptMessage(encryptedMessage: EncryptedMessage): Message {
        const decipher = crypto.createDecipheriv(
            'aes-256-gcm',
            this.secretKey,
            Buffer.from(encryptedMessage.iv, 'hex')
        );

        const decryptedContent = Buffer.concat([
            decipher.update(Buffer.from(encryptedMessage.encryptedContent, 'base64')),
            decipher.final()
        ]).toString('utf8');

        return {
            id: encryptedMessage.id,
            topic: encryptedMessage.topic,
            content: JSON.parse(decryptedContent),
            timestamp: encryptedMessage.timestamp,
            nonce: encryptedMessage.nonce
        };
    }

    // Prüfen auf Message-Replay-Attacken
    private isReplayAttack(message: Message): boolean {
        const messageKey = `${message.id}-${message.nonce}`;
        if (this.processingHistory.has(messageKey)) {
            return true;
        }
        
        // Prüfen ob die Nachricht zu alt ist (älter als 5 Minuten)
        if (Date.now() - message.timestamp > 5 * 60 * 1000) {
            return true;
        }

        this.processingHistory.add(messageKey);
        return false;
    }

    // Nachricht veröffentlichen
    async publish(topic: string, content: any): Promise<void> {
        const message: Message = {
            id: crypto.randomUUID(),
            topic,
            content,
            timestamp: Date.now(),
            nonce: crypto.randomBytes(16).toString('hex')
        };

        if (this.isReplayAttack(message)) {
            throw new Error('Detected potential replay attack');
        }

        const encryptedMessage = this.encryptMessage(message);
        this.messageStore.set(message.id, encryptedMessage);
        await this.persistMessages();

        const subscribers = this.subscribers.get(topic) || new Set();
        for (const subscriber of subscribers) {
            try {
                subscriber(message);
            } catch (error) {
                console.error(`Error delivering message to subscriber: ${error}`);
                this.deadLetterQueue.push(encryptedMessage);
                await this.persistDeadLetterQueue();
            }
        }
    }

    // Abonnieren eines Topics
    subscribe(topic: string, callback: (message: Message) => void): () => void {
        if (!this.subscribers.has(topic)) {
            this.subscribers.set(topic, new Set());
        }
        this.subscribers.get(topic)!.add(callback);

        // Unsubscribe-Funktion zurückgeben
        return () => {
            const topicSubscribers = this.subscribers.get(topic);
            if (topicSubscribers) {
                topicSubscribers.delete(callback);
                if (topicSubscribers.size === 0) {
                    this.subscribers.delete(topic);
                }
            }
        };
    }

    // Persistente Speicherung
    private async persistMessages(): Promise<void> {
        const data = JSON.stringify(Array.from(this.messageStore.entries()));
        await fs.writeFile(`${this.persistencePath}/messages.json`, data);
    }

    private async persistDeadLetterQueue(): Promise<void> {
        const data = JSON.stringify(this.deadLetterQueue);
        await fs.writeFile(`${this.persistencePath}/dlq.json`, data);
    }

    // Laden gespeicherter Nachrichten beim Start
    async loadPersistedData(): Promise<void> {
        try {
            const messagesData = await fs.readFile(`${this.persistencePath}/messages.json`, 'utf8');
            this.messageStore = new Map(JSON.parse(messagesData));

            const dlqData = await fs.readFile(`${this.persistencePath}/dlq.json`, 'utf8');
            this.deadLetterQueue = JSON.parse(dlqData);
        } catch (error) {
            console.warn('No persisted data found, starting with empty state');
        }
    }

    // Nachrichten aus der Dead Letter Queue erneut verarbeiten
    async reprocessDeadLetterQueue(): Promise<void> {
        const messages = [...this.deadLetterQueue];
        this.deadLetterQueue = [];

        for (const encryptedMessage of messages) {
            try {
                const message = this.decryptMessage(encryptedMessage);
                const subscribers = this.subscribers.get(message.topic) || new Set();
                
                for (const subscriber of subscribers) {
                    await subscriber(message);
                }
            } catch (error) {
                console.error(`Error reprocessing message from DLQ: ${error}`);
                this.deadLetterQueue.push(encryptedMessage);
            }
        }

        await this.persistDeadLetterQueue();
    }
}

// Beispielverwendung:
async function main() {
    // Broker mit zufälligem Schlüssel initialisieren
    const secretKey = crypto.randomBytes(32).toString('hex');
    const broker = new MessageBroker(secretKey, './storage');

    // Daten aus der persistenten Speicherung laden
    await broker.loadPersistedData();

    // Topic abonnieren
    const unsubscribe = broker.subscribe('test-topic', (message) => {
        console.log(`Received message: ${JSON.stringify(message)}`);
    });

    // Nachricht veröffentlichen
    await broker.publish('test-topic', { data: 'Hello, World!' });

    // Dead Letter Queue neu verarbeiten
    await broker.reprocessDeadLetterQueue();

    // Abonnement beenden
    unsubscribe();
}

export default MessageBroker;