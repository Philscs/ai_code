// Wire types for Protocol Buffers
const WireType = {
    VARINT: 0,
    LENGTH_DELIMITED: 2
};

class ProtobufWriter {
    constructor() {
        this.buffer = [];
    }

    // Write a varint to the buffer
    writeVarint(value) {
        while (value > 0x7F) {
            this.buffer.push((value & 0x7F) | 0x80);
            value = value >>> 7;
        }
        this.buffer.push(value);
    }

    // Write a field key (tag and wire type)
    writeKey(fieldNumber, wireType) {
        this.writeVarint((fieldNumber << 3) | wireType);
    }

    // Write a string field
    writeString(fieldNumber, value) {
        this.writeKey(fieldNumber, WireType.LENGTH_DELIMITED);
        const bytes = new TextEncoder().encode(value);
        this.writeVarint(bytes.length);
        this.buffer.push(...bytes);
    }

    // Write an int32 field
    writeInt32(fieldNumber, value) {
        this.writeKey(fieldNumber, WireType.VARINT);
        this.writeVarint(value);
    }

    // Write repeated string field
    writeRepeatedString(fieldNumber, values) {
        for (const value of values) {
            this.writeString(fieldNumber, value);
        }
    }

    // Get the final buffer
    getBuffer() {
        return new Uint8Array(this.buffer);
    }
}

class ProtobufReader {
    constructor(buffer) {
        this.buffer = new Uint8Array(buffer);
        this.position = 0;
    }

    // Read a varint from the buffer
    readVarint() {
        let result = 0;
        let shift = 0;

        while (this.position < this.buffer.length) {
            const byte = this.buffer[this.position++];
            result |= (byte & 0x7F) << shift;
            if ((byte & 0x80) === 0) {
                return result;
            }
            shift += 7;
        }

        throw new Error('Malformed varint');
    }

    // Read a string from the buffer
    readString() {
        const length = this.readVarint();
        const bytes = this.buffer.slice(this.position, this.position + length);
        this.position += length;
        return new TextDecoder().decode(bytes);
    }

    // Read a field key
    readKey() {
        const key = this.readVarint();
        return {
            fieldNumber: key >>> 3,
            wireType: key & 0x07
        };
    }

    // Check if we've reached the end of the buffer
    isEnd() {
        return this.position >= this.buffer.length;
    }
}

// Person message serializer
class Person {
    constructor(name = '', age = 0, hobbies = []) {
        this.name = name;
        this.age = age;
        this.hobbies = hobbies;
    }

    // Serialize Person message to buffer
    serialize() {
        const writer = new ProtobufWriter();
        
        if (this.name) {
            writer.writeString(1, this.name);
        }
        
        if (this.age) {
            writer.writeInt32(2, this.age);
        }
        
        if (this.hobbies.length > 0) {
            writer.writeRepeatedString(3, this.hobbies);
        }
        
        return writer.getBuffer();
    }

    // Deserialize buffer to Person message
    static deserialize(buffer) {
        const reader = new ProtobufReader(buffer);
        const person = new Person();

        while (!reader.isEnd()) {
            const { fieldNumber, wireType } = reader.readKey();

            switch (fieldNumber) {
                case 1:
                    if (wireType === WireType.LENGTH_DELIMITED) {
                        person.name = reader.readString();
                    }
                    break;
                case 2:
                    if (wireType === WireType.VARINT) {
                        person.age = reader.readVarint();
                    }
                    break;
                case 3:
                    if (wireType === WireType.LENGTH_DELIMITED) {
                        person.hobbies.push(reader.readString());
                    }
                    break;
                default:
                    throw new Error(`Unknown field number: ${fieldNumber}`);
            }
        }

        return person;
    }
}

// Example usage:
const person = new Person('John Doe', 30, ['reading', 'hiking', 'coding']);
const serialized = person.serialize();
console.log('Serialized:', serialized);

const deserialized = Person.deserialize(serialized);
console.log('Deserialized:', deserialized);