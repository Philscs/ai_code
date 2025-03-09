// Vereinfachter SQL-Parser in JavaScript
class SimpleSQLParser {
    constructor() {
      this.query = {
        select: [],
        from: "",
        where: {},
        groupBy: [],
        having: {}
      };
    }
  
    parse(sql) {
      // Tokenize das SQL-Statement
      const tokens = sql.split(/\s+/);
      
      let currentSection = "";
  
      for (let i = 0; i < tokens.length; i++) {
        const token = tokens[i].toUpperCase();
  
        if (token === "SELECT") {
          currentSection = "select";
        } else if (token === "FROM") {
          currentSection = "from";
        } else if (token === "WHERE") {
          currentSection = "where";
        } else if (token === "GROUP") {
          if (tokens[i + 1].toUpperCase() === "BY") {
            currentSection = "groupBy";
            i++; // Skip "BY"
          }
        } else if (token === "HAVING") {
          currentSection = "having";
        } else {
          // Verarbeite Abschnitte
          if (currentSection === "select") {
            this.query.select.push(token.replace(/,/g, ""));
          } else if (currentSection === "from") {
            this.query.from = token;
          } else if (currentSection === "where") {
            const [field, operator, value] = this.extractCondition(tokens.slice(i));
            this.query.where[field] = { operator, value };
            i += 2; // Skip condition parts
          } else if (currentSection === "groupBy") {
            this.query.groupBy.push(token.replace(/,/g, ""));
          } else if (currentSection === "having") {
            const [field, operator, value] = this.extractCondition(tokens.slice(i));
            this.query.having = { field, operator, value };
            i += 2; // Skip condition parts
          }
        }
      }
  
      return this.query;
    }
  
    extractCondition(tokens) {
      const field = tokens[0];
      const operator = tokens[1];
      const value = tokens[2].replace(/'/g, "");
      return [field, operator, value];
    }
  }
  
  // Beispiel
  const sql = `SELECT name, age FROM users WHERE country = 'DE' GROUP BY age HAVING count > 5`;
  const parser = new SimpleSQLParser();
  const parsedQuery = parser.parse(sql);
  console.log(parsedQuery);
  