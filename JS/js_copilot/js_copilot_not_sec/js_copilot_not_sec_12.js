// BEGIN: Parser
function parseSQL(sql) {
  const tokens = sql.split(" ");
  const query = {};

  if (tokens[0] !== "SELECT") {
    throw new Error("Invalid SQL query. Expected SELECT statement.");
  }

  query.fields = tokens[1].split(",");

  const fromIndex = tokens.indexOf("FROM");
  if (fromIndex === -1) {
    throw new Error("Invalid SQL query. Expected FROM statement.");
  }

  query.table = tokens[fromIndex + 1];

  const whereIndex = tokens.indexOf("WHERE");
  if (whereIndex !== -1) {
    query.where = tokens.slice(whereIndex + 1, fromIndex).join(" ");
  }

  const groupByIndex = tokens.indexOf("GROUP");
  if (groupByIndex !== -1) {
    query.groupBy = tokens[groupByIndex + 2];
  }

  const havingIndex = tokens.indexOf("HAVING");
  if (havingIndex !== -1) {
    query.having = tokens.slice(havingIndex + 1).join(" ");
  }

  return query;
}

// Usage example
const sqlQuery = "SELECT name, age FROM users WHERE country = 'DE' GROUP BY age HAVING count > 5";
const parsedQuery = parseSQL(sqlQuery);
console.log(parsedQuery);
// END: Parser
