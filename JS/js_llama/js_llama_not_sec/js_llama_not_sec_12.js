class SQLParser {
    constructor(query) {
      this.query = query;
      this.parse();
    }
  
    parse() {
      const tokens = this.tokenize(this.query);
      return this.processTokens(tokens);
    }
  
    tokenize(query) {
      const tokens = [];
      let currentToken = '';
  
      for (let i = 0; i < query.length; i++) {
        const char = query[i];
  
        if ([' ', ',', ';', '\n'].includes(char)) {
          if (currentToken !== '') {
            tokens.push(currentToken);
            currentToken = '';
          }
        } else {
          currentToken += char;
        }
      }
  
      if (currentToken !== '') {
        tokens.push(currentToken);
      }
  
      return tokens;
    }
  
    processTokens(tokens) {
      const selectStatement = this.processSelectStatement(tokens);
      return selectStatement;
    }
  
    processSelectStatement(tokens) {
      let selectStatement = [];
  
      // SELECT
      if (tokens[0] !== 'SELECT') {
        throw new Error('Incorrect query');
      }
      selectStatement.push('SELECT');
  
      for (let i = 1; i < tokens.length - 2; i++) {
        const token = tokens[i];
        if (['', ',', ';'].includes(token)) continue;
        if (!this.isIdentifier(token)) throw new Error(`Invalid identifier: ${token}`);
        selectStatement.push(token);
      }
  
      // FROM
      if (tokens[tokens.length - 2] !== 'FROM') {
        throw new Error('Incorrect query');
      }
      selectStatement.push('FROM');
  
      const fromClause = tokens.slice(tokens.length - 1, tokens.length - 1).pop();
      selectStatement.push(fromClause);
  
      // WHERE
      for (let i = tokens.length - 2; i < tokens.length; i++) {
        if (tokens[i] === 'WHERE') {
          selectStatement.push('WHERE');
  
          const condition = this.processCondition(tokens, i);
          if (!condition) return null;
          selectStatement.push(condition);
  
          break;
        }
      }
  
      // GROUP BY
      for (let i = tokens.length - 2; i < tokens.length; i++) {
        if (tokens[i] === 'GROUP BY') {
          selectStatement.push('GROUP BY');
  
          const groupByClause = this.processGroupByClause(tokens, i);
          if (!groupByClause) return null;
          selectStatement.push(groupByClause);
  
          break;
        }
      }
  
      // HAVING
      for (let i = tokens.length - 2; i < tokens.length; i++) {
        if (tokens[i] === 'HAVING') {
          selectStatement.push('HAVING');
  
          const havingClause = this.processHavingClause(tokens, i);
          if (!havingClause) return null;
          selectStatement.push(havingClause);
  
          break;
        }
      }
  
      return selectStatement.join(' ');
    }
  
    processCondition(tokens, index) {
      let condition = tokens[index];
  
      for (let i = index + 1; i < tokens.length; i++) {
        const token = tokens[i];
        if (!this.isIdentifier(token)) continue;
  
        switch (token) {
          case '=':
            return `=`;
          case 'AND':
            return ' AND ';
          case 'OR':
            return ' OR ';
          case '<>':
            return ' != ';
          default:
            break;
        }
      }
  
      return condition;
    }
  
    processGroupByClause(tokens, index) {
      let groupByClause = '';
  
      for (let i = index + 1; i < tokens.length; i++) {
        const token = tokens[i];
        if (!this.isIdentifier(token)) continue;
  
        if (groupByClause === '') {
          groupByClause += token;
        } else {
          groupByClause += ', ' + token;
        }
      }
  
      return groupByClause;
    }
  
    processHavingClause(tokens, index) {
      let havingClause = '';
  
      for (let i = index + 1; i < tokens.length; i++) {
        const token = tokens[i];
        if (!this.isIdentifier(token)) continue;
  
        if (havingClause === '') {
          havingClause += token;
        } else {
          havingClause += ', ' + token;
        }
      }
  
      return havingClause;
    }
  
    isIdentifier(identifier) {
      const regex = /^[a-zA-Z_][a-zA-Z0-9_]*/;
      return regex.test(identifier);
    }
  }
  
  // Beispiel für die Verwendung
  const parser = new SQLParser('SELECT name, age FROM users WHERE country = \'DE\' GROUP BY age HAVING count > 5');
  
  console.log(parser.query); // Ausgabe: SELECT name, age FROM users WHERE country = 'DE' GROUP BY age HAVING count > 5
  
  