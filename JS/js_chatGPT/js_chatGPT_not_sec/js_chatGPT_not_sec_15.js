// Protocol Buffer Parser und Serializer

class ProtoBuf {
    static serializePerson(person) {
      const buffer = [];
  
      // Serialize name (field 1, type: string)
      if (person.name !== undefined) {
        buffer.push(ProtoBuf.encodeKey(1, 2)); // Field number 1, wire type 2 (length-delimited)
        buffer.push(ProtoBuf.encodeString(person.name));
      }
  
      // Serialize age (field 2, type: int32)
      if (person.age !== undefined) {
        buffer.push(ProtoBuf.encodeKey(2, 0)); // Field number 2, wire type 0 (varint)
        buffer.push(ProtoBuf.encodeVarint(person.age));
      }
  
      // Serialize hobbies (field 3, repeated string)
      if (person.hobbies !== undefined) {
        for (const hobby of person.hobbies) {
          buffer.push(ProtoBuf.encodeKey(3, 2)); // Field number 3, wire type 2 (length-delimited)
          buffer.push(ProtoBuf.encodeString(hobby));
        }
      }
  
      return new Uint8Array(buffer.flat());
    }
  
    static deserializePerson(buffer) {
      const person = { hobbies: [] };
      let offset = 0;
  
      while (offset < buffer.length) {
        const [fieldNumber, wireType] = ProtoBuf.decodeKey(buffer[offset++]);
  
        switch (fieldNumber) {
          case 1: // name (string)
            person.name = ProtoBuf.decodeString(buffer, offset);
            offset += person.name.length + ProtoBuf.decodeVarint(buffer, offset - person.name.length - 1).length;
            break;
  
          case 2: // age (int32)
            const ageResult = ProtoBuf.decodeVarint(buffer, offset);
            person.age = ageResult.value;
            offset += ageResult.length;
            break;
  
          case 3: // hobbies (repeated string)
            const hobby = ProtoBuf.decodeString(buffer, offset);
            person.hobbies.push(hobby);
            offset += hobby.length + ProtoBuf.decodeVarint(buffer, offset - hobby.length - 1).length;
            break;
  
          default:
            throw new Error(`Unknown field number: ${fieldNumber}`);
        }
      }
  
      return person;
    }
  
    // Helper Methods
  
    static encodeKey(fieldNumber, wireType) {
      return ProtoBuf.encodeVarint((fieldNumber << 3) | wireType);
    }
  
    static decodeKey(byte) {
      return [byte >> 3, byte & 0x07];
    }
  
    static encodeString(str) {
      const encoded = new TextEncoder().encode(str);
      return [ProtoBuf.encodeVarint(encoded.length), ...encoded];
    }
  
    static decodeString(buffer, offset) {
      const lengthResult = ProtoBuf.decodeVarint(buffer, offset);
      offset += lengthResult.length;
      return new TextDecoder().decode(buffer.slice(offset, offset + lengthResult.value));
    }
  
    static encodeVarint(value) {
      const bytes = [];
      do {
        let byte = value & 0x7F;
        value >>>= 7;
        if (value !== 0) byte |= 0x80;
        bytes.push(byte);
      } while (value !== 0);
      return bytes;
    }
  
    static decodeVarint(buffer, offset) {
      let value = 0;
      let shift = 0;
      let length = 0;
      while (true) {
        const byte = buffer[offset + length++];
        value |= (byte & 0x7F) << shift;
        if ((byte & 0x80) === 0) break;
        shift += 7;
      }
      return { value, length };
    }
  }
  
  // Beispielnutzung
  const person = {
    name: "Max",
    age: 25,
    hobbies: ["Lesen", "Programmieren"]
  };
  
  const serialized = ProtoBuf.serializePerson(person);
  console.log("Serialized:", serialized);
  
  const deserialized = ProtoBuf.deserializePerson(serialized);
  console.log("Deserialized:", deserialized);
  