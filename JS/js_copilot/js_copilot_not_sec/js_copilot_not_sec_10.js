class QueryBuilder {
  constructor(data) {
    this.data = data;
    this.filters = [];
    this.orderByField = null;
    this.groupByField = null;
    this.havingCondition = null;
    this.selectFields = [];
  }

  where(field, operator, value) {
    this.filters.push({ field, operator, value });
    return this;
  }

  orderBy(field) {
    this.orderByField = field;
    return this;
  }

  groupBy(field) {
    this.groupByField = field;
    return this;
  }

  having(field, operator, value) {
    this.havingCondition = { field, operator, value };
    return this;
  }

  select(fields) {
    this.selectFields = fields;
    return this;
  }

  execute() {
    let result = this.data;

    // Apply filters
    for (const filter of this.filters) {
      result = result.filter(item => {
        const fieldValue = item[filter.field];
        switch (filter.operator) {
          case '>':
            return fieldValue > filter.value;
          case '<':
            return fieldValue < filter.value;
          case '>=':
            return fieldValue >= filter.value;
          case '<=':
            return fieldValue <= filter.value;
          case '==':
            return fieldValue === filter.value;
          case '!=':
            return fieldValue !== filter.value;
          default:
            return true;
        }
      });
    }

    // Apply ordering
    if (this.orderByField) {
      result.sort((a, b) => a[this.orderByField].localeCompare(b[this.orderByField]));
    }

    // Apply grouping
    if (this.groupByField) {
      const groups = {};
      for (const item of result) {
        const groupValue = item[this.groupByField];
        if (!groups[groupValue]) {
          groups[groupValue] = [];
        }
        groups[groupValue].push(item);
      }
      result = Object.values(groups);
    }

    // Apply having condition
    if (this.havingCondition) {
      result = result.filter(group => {
        const count = group.length;
        const fieldValue = count;
        switch (this.havingCondition.operator) {
          case '>':
            return fieldValue > this.havingCondition.value;
          case '<':
            return fieldValue < this.havingCondition.value;
          case '>=':
            return fieldValue >= this.havingCondition.value;
          case '<=':
            return fieldValue <= this.havingCondition.value;
          case '==':
            return fieldValue === this.havingCondition.value;
          case '!=':
            return fieldValue !== this.havingCondition.value;
          default:
            return true;
        }
      });
    }

    // Apply selection
    if (this.selectFields.length > 0) {
      result = result.map(item => {
        const selectedFields = {};
        for (const field of this.selectFields) {
          selectedFields[field] = item[field];
        }
        return selectedFields;
      });
    }

    return result;
  }
}

// Usage example
const data = [
  { id: 1, name: 'John', age: 25, country: 'USA' },
  { id: 2, name: 'Alice', age: 30, country: 'USA' },
  { id: 3, name: 'Bob', age: 20, country: 'Canada' },
  { id: 4, name: 'Jane', age: 35, country: 'Canada' },
  { id: 5, name: 'Mike', age: 40, country: 'USA' },
];

const result = new QueryBuilder(data)
  .where('age', '>', 18)
  .orderBy('name')
  .groupBy('country')
  .having('count', '>', 1)
  .select(['id', 'name'])
  .execute();

console.log(result);
