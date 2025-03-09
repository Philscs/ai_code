// JavaScript Interpreter for the DSL
class DSLInterpreter {
    constructor(data) {
      this.data = data;
    }
  
    parse(commandString) {
      const commands = commandString.split('\n').map(line => line.trim()).filter(line => line);
      commands.forEach(command => this.executeCommand(command));
    }
  
    executeCommand(command) {
      const [action, ...params] = command.split(' ');
      switch (action.toUpperCase()) {
        case 'FETCH':
          this.fetchData(params.join(' '));
          break;
        case 'FILTER':
          this.filterData(params.join(' '));
          break;
        case 'SORT':
          this.sortData(params.join(' '));
          break;
        case 'LIMIT':
          this.limitData(parseInt(params[0], 10));
          break;
        default:
          throw new Error(`Unknown command: ${action}`);
      }
    }
  
    async fetchData(endpoint) {
      // Simulating data fetching (in a real case, you would use fetch or axios)
      console.log(`Fetching data from ${endpoint}...`);
      // Assuming data is preloaded for simplicity
    }
  
    filterData(condition) {
      const [field, operator, value] = condition.split(/\s+/);
      const parsedValue = isNaN(value) ? value : Number(value);
  
      switch (operator) {
        case '>':
          this.data = this.data.filter(item => item[field] > parsedValue);
          break;
        case '<':
          this.data = this.data.filter(item => item[field] < parsedValue);
          break;
        case '=':
          this.data = this.data.filter(item => item[field] === parsedValue);
          break;
        default:
          throw new Error(`Unsupported operator: ${operator}`);
      }
      console.log('Filtered data:', this.data);
    }
  
    sortData(order) {
      const [field, direction] = order.split(' ');
      this.data.sort((a, b) => {
        if (a[field] < b[field]) return direction.toUpperCase() === 'ASC' ? -1 : 1;
        if (a[field] > b[field]) return direction.toUpperCase() === 'ASC' ? 1 : -1;
        return 0;
      });
      console.log('Sorted data:', this.data);
    }
  
    limitData(limit) {
      this.data = this.data.slice(0, limit);
      console.log('Limited data:', this.data);
    }
  }
  
  // Example data
  const sampleData = [
    { name: 'Alice', age: 25 },
    { name: 'Bob', age: 17 },
    { name: 'Charlie', age: 19 },
    { name: 'Dave', age: 23 },
    { name: 'Eve', age: 30 },
  ];
  
  // Example usage
  const dsl = new DSLInterpreter(sampleData);
  const commands = `
  FETCH /api/users
  FILTER age > 18
  SORT name ASC
  LIMIT 10
  `;
  dsl.parse(commands);
  
  console.log('Final result:', dsl.data);
  