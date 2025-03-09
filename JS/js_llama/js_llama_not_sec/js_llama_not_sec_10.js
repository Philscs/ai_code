class Query {
    constructor(data) {
      this.data = data;
    }
  
    where(column, operator, value) {
      if (Array.isArray(value)) {
        const operators = {
          'in': (values) => { return `IN (${values.map(() => '?').join(', ')})` },
          'not in': (values) => { return `NOT IN (${values.map(() => '?').join(', ')})` }
        };
        this.whereHelper(column, operators[operator], value);
      } else {
        const values = Array.isArray(value) ? value : [value];
        const placeholder = '?:';
        this.data.where.push({ column, operator, value: '?' });
        this.data.values += values.map(() => placeholder).join(',');
      }
    }
  
    having(column, operator, value) {
      if (Array.isArray(value)) {
        const operators = {
          'in': (values) => { return `IN (${values.map(() => '?').join(', ')})` },
          'not in': (values) => { return `NOT IN (${values.map(() => '?').join(', ')})` }
        };
        this.havingHelper(column, operators[operator], value);
      } else {
        const values = Array.isArray(value) ? value : [value];
        const placeholder = '?:';
        this.data.having.push({ column, operator, value: '?' });
        this.data.values += values.map(() => placeholder).join(',');
      }
    }
  
    havingHelper(column, operator, value) {
      if (operator === 'in' || operator === 'not in') {
        const operators = {
          'in': (values) => { return `(${column} ${operator} (${value}))` },
          'not in': (values) => { return `(${column} NOT ${operator} (${value}))` }
        };
        this.data.having.push({ column, operator: operators[operator], value });
      } else {
        throw new Error(`Unrecognized having operator: ${operator}`);
      }
    }
  
    orderBy(column, direction = 'asc') {
      this.data.orderBy = { column, direction };
    }
  
    groupBy(column) {
      this.data.groupBy = [column];
    }
  
    select(columns) {
      if (!Array.isArray(columns)) {
        columns = [columns];
      }
      this.data.select = columns;
    }
  
    execute() {
      const query = this.data.query;
      console.log(query);
      return this.data.execute(query);
    }
  }
  
  // Beispiel Anwendung
  const data = [
    { id: 1, name: 'John', country: 'USA' },
    { id: 2, name: 'Jane', country: 'Canada' },
    { id: 3, name: 'Bob', country: 'Mexico' },
    // ...
  ];
  
  const query = new Query(data)
    .where('country', 'in', ['USA', 'Canada'])
    .orderBy('name')
    .groupBy('country')
    .having('count(*)', '>', 1)
    .select(['id', 'name'])
    .execute();