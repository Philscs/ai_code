// Tokenizer
class Token {
    constructor(type, value) {
        this.type = type;
        this.value = value;
    }
}

class Lexer {
    constructor(code) {
        this.code = code.split("\n");
        this.currentTokenIndex = 0;
    }

    tokenize() {
        const tokens = [];
        while (this.currentTokenIndex < this.code.length) {
            if (this.code[this.currentTokenIndex].trim() === "") {
                // Leerzeichen
                this.currentTokenIndex++;
                continue;
            }

            if (this.code[this.currentTokenIndex].startsWith("/")) {
                // Kommentar
                const commentEnd = this.code[this.currentTokenIndex].indexOf("\n");
                if (commentEnd !== -1) {
                    tokens.push(new Token("COMMENT", this.code[this.currentTokenIndex].slice(1, 
commentEnd)));
                    this.currentTokenIndex += commentEnd + 1;
                } else {
                    tokens.push(new Token("COMMENT", 
this.code[this.currentTokenIndex].slice(1)));
                    this.currentTokenIndex++;
                }
            }

            if (this.code[this.currentTokenIndex] === "FETCH") {
                tokens.push(new Token("KEYWORD", "FETCH"));
                this.currentTokenIndex++;
            } else if (this.code[this.currentTokenIndex].startsWith("http://")) {
                // URL
                const urlEnd = this.code[this.currentTokenIndex].indexOf(" ");
                tokens.push(new Token("URL", this.code[this.currentTokenIndex].slice(7, 
urlEnd)));
                this.currentTokenIndex += urlEnd + 1;
            } else if (this.code[this.currentTokenIndex] === "/api/users") {
                // API-Endpoint
                tokens.push(new Token("KEYWORD", "/api/users"));
                this.currentTokenIndex++;
            } else if (this.code[this.currentTokenIndex].startsWith("/")) {
                // Filter-Operator
                const operatorEnd = this.code[this.currentTokenIndex].indexOf(" ");
                tokens.push(new Token("FILTER", this.code[this.currentTokenIndex].slice(1, 
operatorEnd)));
                this.currentTokenIndex += operatorEnd + 1;
            } else if (this.code[this.currentTokenIndex] === "AGE") {
                // Filter-Operator
                tokens.push(new Token("KEYWORD", "AGE"));
                this.currentTokenIndex++;
            } else if (this.code[this.currentTokenIndex].startsWith(">")) {
                // Operator
                const operatorEnd = this.code[this.currentTokenIndex].indexOf(" ");
                tokens.push(new Token("OP", this.code[this.currentTokenIndex].slice(1, 
operatorEnd)));
                this.currentTokenIndex += operatorEnd + 1;
            } else if (this.code[this.currentTokenIndex] === ">") {
                // Operator
                tokens.push(new Token("OP", ">"));
                this.currentTokenIndex++;
            } else if (this.code[this.currentTokenIndex].startsWith("=")) {
                // Operator
                const operatorEnd = this.code[this.currentTokenIndex].indexOf(" ");
                tokens.push(new Token("OP", this.code[this.currentTokenIndex].slice(1, 
operatorEnd)));
                this.currentTokenIndex += operatorEnd + 1;
            } else if (this.code[this.currentTokenIndex] === "=") {
                // Operator
                tokens.push(new Token("OP", "="));
                this.currentTokenIndex++;
            } else if (this.code[this.currentTokenIndex].startsWith("SORT")) {
                // SORT-Operator
                const operatorEnd = this.code[this.currentTokenIndex].indexOf(" ");
                tokens.push(new Token("KEYWORD", "SORT"));
                this.currentTokenIndex += operatorEnd + 1;
            } else if (this.code[this.currentTokenIndex] === "name") {
                // Filter-Operator
                tokens.push(new Token("KEYWORD", "name"));
                this.currentTokenIndex++;
            } else if (this.code[this.currentTokenIndex].startsWith("ASC")) {
                // SORT-Direktion
                const operatorEnd = this.code[this.currentTokenIndex].indexOf(" ");
                tokens.push(new Token("KEYWORD", "ASC"));
                this.currentTokenIndex += operatorEnd + 1;
            } else if (this.code[this.currentTokenIndex] === "name ASC") {
                // Filter-Operator
                tokens.push(new Token("FILTER", "name ASC"));
                this.currentTokenIndex++;
            } else if (this.code[this.currentTokenIndex].startsWith("LIMIT")) {
                // LIMIT Operator
                const operatorEnd = this.code[this.currentTokenIndex].indexOf(" ");
                tokens.push(new Token("KEYWORD", "LIMIT"));
                this.currentTokenIndex += operatorEnd + 1;
            } else if (this.code[this.currentTokenIndex] === "10") {
                // LIMIT-Wert
                tokens.push(new Token("VALUE", "10"));
                this.currentTokenIndex++;
            }
        }

        return tokens;
    }
}

// Interpreter
class Interpreter {
    constructor(tokens) {
        this.tokens = tokens;
        this.index = 0;
        this.result = [];
    }

    parse() {
        while (this.index < this.tokens.length) {
            if (this.tokens[this.index].type === "KEYWORD") {
                switch (this.tokens[this.index].value) {
                    case "FETCH":
                        this.parseFetch();
                        break;
                    case "/api/users":
                        // API-Endpoint
                        this.result.push("Fetching users from /api/users");
                        break;
                    default:
                        throw new Error(`Ungültige Keyword: ${this.tokens[this.index].value}`);
                }
            } else if (this.tokens[this.index].type === "FILTER") {
                switch (this.tokens[this.index].value) {
                    case "age >":
                        this.parseAgeFilter();
                        break;
                    default:
                        throw new Error(`Ungültige Filter: ${this.tokens[this.index].value}`);
                }
            } else if (this.tokens[this.index].type === "OP") {
                switch (this.tokens[this.index].value) {
                    case ">":
                        // Operator
                        this.parseOperator();
                        break;
                    default:
                        throw new Error(`Ungültiger Operator: ${this.tokens[this.index].value}`);
                }
            } else if (this.tokens[this.index].type === "VALUE") {
                // LIMIT-Wert
                this.result.push(this.tokens[this.index].value);
            } else if (this.tokens[this.index].type === "COMMENT") {
                // Kommentar
                continue;
            }

            this.index++;
        }
    }

    parseFetch() {
        this.result.push("Fetching users");
    }

    parseAgeFilter() {
        // Filter-Operator
        this.result.push(this.tokens[this.index - 1].value);
        if (this.tokens[this.index].type === "OP" && this.tokens[this.index].value === ">") {
            // Operator
            this.parseOperator();
        }
    }

    parseOperator() {
        // Operator
        this.result.push(`>= ${this.tokens[this.index - 2].value}`);
        this.index += 3;
    }
}

// Ausgabe
function main(code) {
    const lexer = new Lexer(code);
    const tokens = lexer.tokenize();

    if (tokens.length === 0) {
        console.log("Keine gültigen Statemente gefunden.");
        return;
    }

    const interpreter = new Interpreter(tokens);
    interpreter.parse();
}

const code = `FETCH /api/users
FILTER age > 18
SORT name ASC
LIMIT 10`;
main(code);
