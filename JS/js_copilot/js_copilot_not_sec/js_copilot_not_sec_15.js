// BEGIN: Protocol Buffer Parser
function parsePerson(buffer) {
  const person = Person.decode(buffer);
  return {
    name: person.name,
    age: person.age,
    hobbies: person.hobbies,
  };
}
// END: Protocol Buffer Parser

// BEGIN: Protocol Buffer Serializer
function serializePerson(person) {
  const message = new Person();
  message.name = person.name;
  message.age = person.age;
  message.hobbies = person.hobbies;
  return Person.encode(message).finish();
}
// END: Protocol Buffer Serializer
