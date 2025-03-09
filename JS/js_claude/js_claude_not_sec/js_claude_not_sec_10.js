class QueryBuilder {
    constructor(data) {
      this.data = data;
      this.operations = [];
    }
  
    where(field, operator, value) {
      const operators = {
        '>': (a, b) => a > b,
        '<': (a, b) => a < b,
        '>=': (a, b) => a >= b,
        '<=': (a, b) => a <= b,
        '=': (a, b) => a === b,
        '!=': (a, b) => a !== b,
      };
  
      this.operations.push(data => 
        data.filter(item => operators[operator](item[field], value))
      );
      return this;
    }
  
    orderBy(field, direction = 'asc') {
      this.operations.push(data => 
        [...data].sort((a, b) => {
          if (direction.toLowerCase() === 'desc') {
            return a[field] > b[field] ? -1 : 1;
          }
          return a[field] > b[field] ? 1 : -1;
        })
      );
      return this;
    }
  
    groupBy(field) {
      this.operations.push(data => {
        const groups = {};
        data.forEach(item => {
          const key = item[field];
          if (!groups[key]) {
            groups[key] = [];
          }
          groups[key].push(item);
        });
        return Object.values(groups);
      });
      return this;
    }
  
    having(aggregateField, operator, value) {
      const operators = {
        '>': (a, b) => a > b,
        '<': (a, b) => a < b,
        '>=': (a, b) => a >= b,
        '<=': (a, b) => a <= b,
        '=': (a, b) => a === b,
        '!=': (a, b) => a !== b,
      };
  
      const aggregates = {
        'count': group => group.length,
        'sum': (group, field) => group.reduce((sum, item) => sum + item[field], 0),
        'avg': (group, field) => group.reduce((sum, item) => sum + item[field], 0) / group.length,
        'min': (group, field) => Math.min(...group.map(item => item[field])),
        'max': (group, field) => Math.max(...group.map(item => item[field])),
      };
  
      this.operations.push(data => 
        data.filter(group => operators[operator](
          aggregates[aggregateField](group),
          value
        ))
      );
      return this;
    }
  
    select(fields) {
      this.operations.push(data => {
        if (Array.isArray(data[0])) {
          // Handle grouped data
          return data.map(group => 
            group.map(item => 
              fields.reduce((obj, field) => {
                obj[field] = item[field];
                return obj;
              }, {})
            )
          );
        }
        
        return data.map(item => 
          fields.reduce((obj, field) => {
            obj[field] = item[field];
            return obj;
          }, {})
        );
      });
      return this;
    }
  
    execute() {
      return this.operations.reduce((result, operation) => operation(result), this.data);
    }
  }
  
  // Helper function to create new query instance
  function query(data) {
    return new QueryBuilder(data);
  }
  
  // Example usage:
  const data = [
    { id: 1, name: 'John', age: 25, country: 'USA' },
    { id: 2, name: 'Alice', age: 30, country: 'Canada' },
    { id: 3, name: 'Bob', age: 17, country: 'USA' },
    { id: 4, name: 'Carol', age: 22, country: 'Canada' },
    { id: 5, name: 'Dave', age: 28, country: 'USA' },
  ];
  
  const result = query(data)
    .where('age', '>', 18)
    .orderBy('name')
    .groupBy('country')
    .having('count', '>', 1)
    .select(['id', 'name'])
    .execute();
  
  console.log(result);