class DSLInterpreter {
    constructor() {
        this.operations = [];
    }

    parse(dslString) {
        // Split input into lines and remove empty lines
        const lines = dslString.trim().split('\n').filter(line => line.trim());
        
        for (const line of lines) {
            const [command, ...args] = line.trim().split(' ');
            
            switch (command.toUpperCase()) {
                case 'FETCH':
                    this.operations.push({
                        type: 'fetch',
                        endpoint: args.join(' ')
                    });
                    break;
                    
                case 'FILTER':
                    const condition = args.join(' ');
                    const [field, operator, value] = this.parseFilterCondition(condition);
                    this.operations.push({
                        type: 'filter',
                        field,
                        operator,
                        value
                    });
                    break;
                    
                case 'SORT':
                    const [field2, direction] = args;
                    this.operations.push({
                        type: 'sort',
                        field: field2,
                        direction: direction?.toUpperCase() || 'ASC'
                    });
                    break;
                    
                case 'LIMIT':
                    this.operations.push({
                        type: 'limit',
                        value: parseInt(args[0])
                    });
                    break;
                    
                default:
                    throw new Error(`Unknown command: ${command}`);
            }
        }
        
        return this;
    }

    parseFilterCondition(condition) {
        // Support for >, <, >=, <=, ==, !=
        const operators = ['>=', '<=', '>', '<', '==', '!='];
        let operator = '';
        let [field, value] = ['', ''];

        for (const op of operators) {
            if (condition.includes(op)) {
                operator = op;
                [field, value] = condition.split(op).map(s => s.trim());
                break;
            }
        }

        // Try to convert value to number if possible
        const numValue = Number(value);
        value = isNaN(numValue) ? value : numValue;

        return [field, operator, value];
    }

    async execute() {
        let data = [];
        
        for (const operation of this.operations) {
            switch (operation.type) {
                case 'fetch':
                    data = await this.executeFetch(operation.endpoint);
                    break;
                    
                case 'filter':
                    data = this.executeFilter(data, operation);
                    break;
                    
                case 'sort':
                    data = this.executeSort(data, operation);
                    break;
                    
                case 'limit':
                    data = this.executeLimit(data, operation);
                    break;
            }
        }
        
        return data;
    }

    async executeFetch(endpoint) {
        try {
            const response = await fetch(endpoint);
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            return await response.json();
        } catch (error) {
            console.error('Fetch error:', error);
            throw error;
        }
    }

    executeFilter(data, operation) {
        return data.filter(item => {
            const itemValue = item[operation.field];
            const compareValue = operation.value;
            
            switch (operation.operator) {
                case '>': return itemValue > compareValue;
                case '<': return itemValue < compareValue;
                case '>=': return itemValue >= compareValue;
                case '<=': return itemValue <= compareValue;
                case '==': return itemValue === compareValue;
                case '!=': return itemValue !== compareValue;
                default: return false;
            }
        });
    }

    executeSort(data, operation) {
        return [...data].sort((a, b) => {
            const valueA = a[operation.field];
            const valueB = b[operation.field];
            
            const direction = operation.direction === 'ASC' ? 1 : -1;
            
            if (valueA < valueB) return -1 * direction;
            if (valueA > valueB) return 1 * direction;
            return 0;
        });
    }

    executeLimit(data, operation) {
        return data.slice(0, operation.value);
    }
}

// Example usage:
const dsl = `
FETCH /api/users
FILTER age > 18
SORT name ASC
LIMIT 10
`;

const interpreter = new DSLInterpreter();

// Parse and execute
async function runDSL() {
    try {
        const result = await interpreter
            .parse(dsl)
            .execute();
        console.log('Result:', result);
    } catch (error) {
        console.error('Error executing DSL:', error);
    }
}