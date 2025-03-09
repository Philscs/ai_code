class Person {
    constructor(name, age) {
      this.name = name;
      this.age = age;
      this.hobbies = [];
    }
  
    addHobby(hobby) {
      if (!this.hobbies.includes(hobby)) {
        this.hobbies.push(hobby);
      }
    }
  }
  
  function parse(personData) {
    // Verifiziere, ob die Person-Datei in Protobuf-Format ist
    if (typeof personData === 'object') {
      const { name, age } = personData;
  
      return new Person(name, age);
    }
  
    return null;
  }
  
  function serialize(person) {
    const data = {};
  
    // Add Name
    data.name = person.name;
  
    // Add Alter
    data.age = person.age;
  
    // Add Hobbys
    if (person.hobbies.length > 0) {
      data.hobbies = person.hobbies.map(hobby => hobby.toString());
    }
  
    return JSON.stringify(data);
  }
  
  // Beispiel für die Verwendung:
  const serializedData = serialize(new Person("Max Mustermann", 32));
  console.log(serializedData);
  
  // Um den Protobuf-Format zu parsen, müssen wir JSON zu Protobuf zurückübersetzen.
  const personData = parse(JSON.parse(serializedData));
  
  if (personData) {
    console.log(personData.name);
    console.log(personData.age);
    console.log(personData.hobbies);
  }
  class Person {
    constructor(name, age) {
      this.name = name;
      this.age = age;
      this.hobbies = [];
    }
  
    addHobby(hobby) {
      if (!this.hobbies.includes(hobby)) {
        this.hobbies.push(hobby);
      }
    }
  }
  
  const protobufSchema = {
    person: {
      type: 'message',
      fields: [
        { name: 'name', number: 1 },
        { name: 'age', number: 2 },
        { name: 'hobbies', repeated: true, fieldNumber: 3 },
      ],
    }
  };
  
  function protobufParser(data) {
    const parser = new protobuf.Parser();
    return parser.parse(data);
  }
  
  // Beispiel für die Verwendung:
  const serializedData = JSON.stringify({
    person: {
      name: "Max Mustermann",
      age: 32,
      hobbies: ["Sport", "Musik", "Fahrradfahren"],
    }
  });
  
  const personData = protobufParser(serializedData);
  
  if (personData) {
    console.log(personData.person.name);
    console.log(personData.person.age);
    console.log(personData.person.hobbies);
  }
  