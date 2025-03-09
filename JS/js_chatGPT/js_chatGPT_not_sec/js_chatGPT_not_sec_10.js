class QueryBuilder {
    constructor(data) {
      this.data = data;
      this.query = { where: [], orderBy: null, groupBy: null, having: [], select: [] };
    }
  
    where(field, operator, value) {
      this.query.where.push({ field, operator, value });
      return this;
    }
  
    orderBy(field, order = 'asc') {
      this.query.orderBy = { field, order };
      return this;
    }
  
    groupBy(field) {
      this.query.groupBy = field;
      return this;
    }
  
    having(field, operator, value) {
      this.query.having.push({ field, operator, value });
      return this;
    }
  
    select(fields) {
      this.query.select = fields;
      return this;
    }
  
    execute() {
      let result = [...this.data];
  
      // Apply where clauses
      this.query.where.forEach(({ field, operator, value }) => {
        result = result.filter(item => {
          switch (operator) {
            case '>': return item[field] > value;
            case '<': return item[field] < value;
            case '>=': return item[field] >= value;
            case '<=': return item[field] <= value;
            case '==': return item[field] == value;
            case '!=': return item[field] != value;
            default: throw new Error(`Unsupported operator: ${operator}`);
          }
        });
      });
  
      // Apply groupBy
      if (this.query.groupBy) {
        const grouped = {};
        result.forEach(item => {
          const key = item[this.query.groupBy];
          if (!grouped[key]) grouped[key] = [];
          grouped[key].push(item);
        });
        result = Object.keys(grouped).map(key => ({
          [this.query.groupBy]: key,
          items: grouped[key],
          count: grouped[key].length
        }));
      }
  
      // Apply having
      this.query.having.forEach(({ field, operator, value }) => {
        result = result.filter(item => {
          switch (operator) {
            case '>': return item[field] > value;
            case '<': return item[field] < value;
            case '>=': return item[field] >= value;
            case '<=': return item[field] <= value;
            case '==': return item[field] == value;
            case '!=': return item[field] != value;
            default: throw new Error(`Unsupported operator: ${operator}`);
          }
        });
      });
  
      // Apply orderBy
      if (this.query.orderBy) {
        const { field, order } = this.query.orderBy;
        result.sort((a, b) => {
          if (a[field] < b[field]) return order === 'asc' ? -1 : 1;
          if (a[field] > b[field]) return order === 'asc' ? 1 : -1;
          return 0;
        });
      }
  
      // Apply select
      if (this.query.select.length > 0) {
        result = result.map(item => {
          const selected = {};
          this.query.select.forEach(field => {
            selected[field] = item[field];
          });
          return selected;
        });
      }
  
      return result;
    }
  }
  
  // Usage
  const data = [
    { id: 1, name: 'Alice', age: 25, country: 'USA' },
    { id: 2, name: 'Bob', age: 17, country: 'USA' },
    { id: 3, name: 'Charlie', age: 23, country: 'UK' },
    { id: 4, name: 'David', age: 30, country: 'UK' },
    { id: 5, name: 'Eve', age: 22, country: 'USA' },
  ];
  
  const result = new QueryBuilder(data)
    .where('age', '>', 18)
    .orderBy('name')
    .groupBy('country')
    .having('count', '>', 1)
    .select(['country', 'count'])
    .execute();
  
  console.log(result);