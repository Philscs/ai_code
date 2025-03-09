// BEGIN: Interpreter
class DSLInterpreter {
  constructor() {
    this.data = [];
  }

  fetch(url) {
    // Simulate fetching data from the API
    this.data = [
      { name: "John", age: 25 },
      { name: "Alice", age: 20 },
      { name: "Bob", age: 30 },
      // ...
    ];
    return this;
  }

  filter(condition) {
    // Apply filter based on the condition
    this.data = this.data.filter((item) => {
      const [property, operator, value] = condition.split(" ");
      switch (operator) {
        case ">":
          return item[property] > value;
        case "<":
          return item[property] < value;
        case ">=":
          return item[property] >= value;
        case "<=":
          return item[property] <= value;
        case "==":
          return item[property] == value;
        case "!=":
          return item[property] != value;
        default:
          return true;
      }
    });
    return this;
  }

  sort(property, order) {
    // Sort the data based on the property and order
    this.data.sort((a, b) => {
      if (order === "ASC") {
        return a[property] > b[property] ? 1 : -1;
      } else {
        return a[property] < b[property] ? 1 : -1;
      }
    });
    return this;
  }

  limit(count) {
    // Limit the number of results
    this.data = this.data.slice(0, count);
    return this;
  }

  execute() {
    // Return the final result
    return this.data;
  }
}

// Usage example
const interpreter = new DSLInterpreter();
const result = interpreter
  .fetch("/api/users")
  .filter("age > 18")
  .sort("name", "ASC")
  .limit(10)
  .execute();

console.log(result);
// END: Interpreter
