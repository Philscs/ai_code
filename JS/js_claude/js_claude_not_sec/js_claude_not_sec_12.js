class SQLParser {
    constructor() {
      this.query = {
        select: [],
        from: '',
        where: null,
        groupBy: null,
        having: null
      };
    }
  
    parse(sqlString) {
      // Entferne überflüssige Leerzeichen und Zeilenumbrüche
      const normalizedSQL = sqlString.replace(/\s+/g, ' ').trim();
      
      // Teile den SQL-String in seine Hauptkomponenten
      const parts = normalizedSQL.split(/\b(SELECT|FROM|WHERE|GROUP BY|HAVING)\b/i);
      
      // Verarbeite die einzelnen Teile
      for (let i = 0; i < parts.length; i++) {
        const part = parts[i].trim();
        switch (part.toUpperCase()) {
          case 'SELECT':
            this.parseSelect(parts[i + 1]);
            break;
          case 'FROM':
            this.parseFrom(parts[i + 1]);
            break;
          case 'WHERE':
            this.parseWhere(parts[i + 1]);
            break;
          case 'GROUP BY':
            this.parseGroupBy(parts[i + 1]);
            break;
          case 'HAVING':
            this.parseHaving(parts[i + 1]);
            break;
        }
      }
      
      return this.query;
    }
  
    parseSelect(selectClause) {
      if (!selectClause) return;
      // Entferne Kommas und teile nach Leerzeichen
      this.query.select = selectClause
        .split(',')
        .map(field => field.trim())
        .filter(field => field.length > 0);
    }
  
    parseFrom(fromClause) {
      if (!fromClause) return;
      this.query.from = fromClause.trim().split(' ')[0];
    }
  
    parseWhere(whereClause) {
      if (!whereClause) return;
      // Einfache Implementierung für Gleichheitsvergleiche
      const condition = whereClause.trim();
      if (condition.includes('=')) {
        const [field, value] = condition.split('=').map(part => part.trim());
        this.query.where = {
          field,
          operator: '=',
          value: value.replace(/'/g, '').trim() // Entferne Anführungszeichen
        };
      }
    }
  
    parseGroupBy(groupByClause) {
      if (!groupByClause) return;
      this.query.groupBy = groupByClause.trim();
    }
  
    parseHaving(havingClause) {
      if (!havingClause) return;
      // Einfache Implementierung für Vergleiche
      const condition = havingClause.trim();
      if (condition.includes('>')) {
        const [field, value] = condition.split('>').map(part => part.trim());
        this.query.having = {
          field,
          operator: '>',
          value: parseInt(value)
        };
      }
    }
  }
  
  // Beispielverwendung:
  const parser = new SQLParser();
  const sqlQuery = `
    SELECT name, age 
    FROM users 
    WHERE country = 'DE' 
    GROUP BY age 
    HAVING count > 5
  `;
  
  const parsedQuery = parser.parse(sqlQuery);
  console.log(JSON.stringify(parsedQuery, null, 2));
  
  /* Beispiel-Output:
  {
    "select": ["name", "age"],
    "from": "users",
    "where": {
      "field": "country",
      "operator": "=",
      "value": "DE"
    },
    "groupBy": "age",
    "having": {
      "field": "count",
      "operator": ">",
      "value": 5
    }
  }
  */