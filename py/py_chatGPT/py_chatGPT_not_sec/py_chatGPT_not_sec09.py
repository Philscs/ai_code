import re

# Schritt 1: Lexer
class Lexer:
    def __init__(self, code):
        self.code = code
        self.tokens = []

    def tokenize(self):
        token_specification = [
            ("NUMBER",    r'\d+'),
            ("IDENT",     r'[a-zA-Z_][a-zA-Z_0-9]*'),
            ("ASSIGN",    r'='),
            ("END",       r';'),
            ("OP",        r'[+\-*/]'),
            ("LPAREN",    r'\('),
            ("RPAREN",    r'\)'),
            ("WHITESPACE",r'\s+'),
            ("IF",        r'if'),
            ("ELSE",      r'else'),
            ("WHILE",     r'while'),
            ("PRINT",     r'print'),
        ]
        tok_regex = '|'.join(f'(?P<{pair[0]}>{pair[1]})' for pair in token_specification)
        for mo in re.finditer(tok_regex, self.code):
            kind = mo.lastgroup
            value = mo.group()
            if kind == "WHITESPACE":
                continue
            self.tokens.append((kind, value))
        return self.tokens

# Schritt 2: Parser
class Parser:
    def __init__(self, tokens):
        self.tokens = tokens
        self.current_token = None
        self.pos = -1
        self.advance()

    def advance(self):
        self.pos += 1
        self.current_token = self.tokens[self.pos] if self.pos < len(self.tokens) else None

    def parse(self):
        statements = []
        while self.current_token is not None:
            statements.append(self.statement())
        return statements

    def statement(self):
        if self.current_token[0] == "IDENT":
            return self.assignment()
        elif self.current_token[0] == "PRINT":
            return self.print_statement()
        else:
            raise SyntaxError(f"Unexpected token {self.current_token}")

    def assignment(self):
        var_name = self.current_token[1]
        self.advance()
        if self.current_token[0] != "ASSIGN":
            raise SyntaxError("Expected '='")
        self.advance()
        expr = self.expression()
        if self.current_token[0] != "END":
            raise SyntaxError("Expected ';'")
        self.advance()
        return ("ASSIGN", var_name, expr)

    def print_statement(self):
        self.advance()
        expr = self.expression()
        if self.current_token[0] != "END":
            raise SyntaxError("Expected ';'")
        self.advance()
        return ("PRINT", expr)

    def expression(self):
        term = self.term()
        while self.current_token and self.current_token[0] == "OP":
            op = self.current_token[1]
            self.advance()
            right = self.term()
            term = (op, term, right)
        return term

    def term(self):
        token = self.current_token
        if token[0] == "NUMBER":
            self.advance()
            return ("NUMBER", int(token[1]))
        elif token[0] == "IDENT":
            self.advance()
            return ("IDENT", token[1])
        elif token[0] == "LPAREN":
            self.advance()
            expr = self.expression()
            if self.current_token[0] != "RPAREN":
                raise SyntaxError("Expected ')'")
            self.advance()
            return expr
        else:
            raise SyntaxError(f"Unexpected token {token}")

# Schritt 3: Semantische Analyse
class SemanticAnalyzer:
    def __init__(self):
        self.symbol_table = {}

    def analyze(self, statements):
        for stmt in statements:
            if stmt[0] == "ASSIGN":
                var_name = stmt[1]
                expr = stmt[2]
                self.evaluate_expression(expr)
                self.symbol_table[var_name] = expr

    def evaluate_expression(self, expr):
        if expr[0] == "NUMBER":
            return expr[1]
        elif expr[0] == "IDENT":
            if expr[1] not in self.symbol_table:
                raise NameError(f"Variable {expr[1]} not defined")
            return self.symbol_table[expr[1]]

# Schritt 4: Codegenerator
class CodeGenerator:
    def __init__(self):
        self.output = []

    def generate(self, statements):
        for stmt in statements:
            if stmt[0] == "ASSIGN":
                self.output.append(f"{stmt[1]} = {self.evaluate(stmt[2])}")
            elif stmt[0] == "PRINT":
                self.output.append(f"print({self.evaluate(stmt[1])})")
        return '\n'.join(self.output)

    def evaluate(self, expr):
        if expr[0] == "NUMBER":
            return str(expr[1])
        elif expr[0] == "IDENT":
            return expr[1]
        elif expr[0] in {"+", "-", "*", "/"}:
            left = self.evaluate(expr[1])
            right = self.evaluate(expr[2])
            return f"({left} {expr[0]} {right})"

# Beispiel: Nutzung des Compilers
if __name__ == "__main__":
    code = "x = 5; y = x + 10; print(y);"

    lexer = Lexer(code)
    tokens = lexer.tokenize()

    parser = Parser(tokens)
    ast = parser.parse()

    analyzer = SemanticAnalyzer()
    analyzer.analyze(ast)

    generator = CodeGenerator()
    output = generator.generate(ast)

    print("\nGenerierter Code:")
    print(output)
