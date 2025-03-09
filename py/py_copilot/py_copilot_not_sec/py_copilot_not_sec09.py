from enum import Enum, auto
from dataclasses import dataclass
from typing import List, Dict, Any, Optional
import ast
from enum import Enum
from dataclasses import dataclass
from typing import List, Dict, Any

# Token-Typen
class TokenType(Enum):
    NUMBER = auto()
    IDENTIFIER = auto()
    PLUS = auto()
    MINUS = auto()
    MULTIPLY = auto()
    DIVIDE = auto()
    EQUALS = auto()
    LPAREN = auto()
    RPAREN = auto()
    LBRACE = auto()
    RBRACE = auto()
    SEMICOLON = auto()
    IF = auto()
    WHILE = auto()
    FUNCTION = auto()
    RETURN = auto()
    EOF = auto()

@dataclass
class Token:
    type: TokenType
    value: str
    line: int
    column: int

# Lexer
class Lexer:
    def __init__(self, source: str):
        self.source = source
        self.pos = 0
        self.line = 1
        self.column = 1
        self.current_char = self.source[0] if source else None

    def advance(self):
        self.pos += 1
        if self.pos >= len(self.source):
            self.current_char = None
        else:
            if self.current_char == '\n':
                self.line += 1
                self.column = 1
            else:
                self.column += 1
            self.current_char = self.source[self.pos]

    def skip_whitespace(self):
        while self.current_char and self.current_char.isspace():
            self.advance()

    def number(self) -> Token:
        result = ''
        while self.current_char and self.current_char.isdigit():
            result += self.current_char
            self.advance()
        return Token(TokenType.NUMBER, result, self.line, self.column)

    def identifier(self) -> Token:
        result = ''
        while self.current_char and (self.current_char.isalnum() or self.current_char == '_'):
            result += self.current_char
            self.advance()
        
        # Schlüsselwörter prüfen
        keywords = {
            'if': TokenType.IF,
            'while': TokenType.WHILE,
            'function': TokenType.FUNCTION,
        }

        # Token types
        class TokenType(Enum):
            NUMBER = "NUMBER"
            PLUS = "PLUS"
            MINUS = "MINUS"
            MULTIPLY = "MULTIPLY"
            DIVIDE = "DIVIDE"
            EQUALS = "EQUALS"
            LPAREN = "LPAREN"
            RPAREN = "RPAREN"
            LBRACE = "LBRACE"
            RBRACE = "RBRACE"
            SEMICOLON = "SEMICOLON"
            IF = "IF"
            WHILE = "WHILE"
            FUNCTION = "FUNCTION"
            RETURN = "RETURN"
            IDENTIFIER = "IDENTIFIER"
            EOF = "EOF"


        # Token class
        @dataclass
        class Token:
            type: TokenType
            value: str
            line: int
            column: int


        # Lexer class
        class Lexer:
            def __init__(self, text: str):
                self.text = text
                self.pos = 0
                self.line = 1
                self.column = 1
                self.current_char = self.text[self.pos] if self.pos < len(self.text) else None

            def advance(self):
                self.pos += 1
                self.column += 1
                if self.pos < len(self.text):
                    self.current_char = self.text[self.pos]
                else:
                    self.current_char = None

            def skip_whitespace(self):
                while self.current_char and self.current_char.isspace():
                    if self.current_char == '\n':
                        self.line += 1
                        self.column = 1
                    self.advance()

            def number(self):
                result = ''
                while self.current_char and self.current_char.isdigit():
                    result += self.current_char
                    self.advance()
                return int(result)

            def identifier(self):
                result = ''
                while self.current_char and (self.current_char.isalpha() or self.current_char.isdigit() or self.current_char == '_'):
                    result += self.current_char
                    self.advance()
                return result

            def get_next_token(self) -> Token:
                while self.current_char:
                    if self.current_char.isspace():
                        self.skip_whitespace()
                        continue

                    if self.current_char.isdigit():
                        return Token(TokenType.NUMBER, self.number(), self.line, self.column)

                    if self.current_char.isalpha() or self.current_char == '_':
                        return Token(TokenType.IDENTIFIER, self.identifier(), self.line, self.column)

                    char_to_token = {
                        '+': TokenType.PLUS,
                        '-': TokenType.MINUS,
                        '*': TokenType.MULTIPLY,
                        '/': TokenType.DIVIDE,
                        '=': TokenType.EQUALS,
                        '(': TokenType.LPAREN,
                        ')': TokenType.RPAREN,
                        '{': TokenType.LBRACE,
                        '}': TokenType.RBRACE,
                        ';': TokenType.SEMICOLON
                    }

                    if self.current_char in char_to_token:
                        token = Token(char_to_token[self.current_char], self.current_char, self.line, self.column)
                        self.advance()
                        return token

                    raise SyntaxError(f"Unexpected character '{self.current_char}' at line {self.line}, column {self.column}")

                return Token(TokenType.EOF, '', self.line, self.column)


        # AST nodes
        class ASTNode:
            pass


        @dataclass
        class NumberNode(ASTNode):
            value: int


        @dataclass
        class BinOpNode(ASTNode):
            left: ASTNode
            op: TokenType
            right: ASTNode


        @dataclass
        class AssignmentNode(ASTNode):
            name: str
            value: ASTNode


        @dataclass
        class VariableNode(ASTNode):
            name: str


        @dataclass
        class IfNode(ASTNode):
            condition: ASTNode
            body: List[ASTNode]


        @dataclass
        class WhileNode(ASTNode):
            condition: ASTNode
            body: List[ASTNode]


        @dataclass
        class FunctionNode(ASTNode):
            name: str
            params: List[str]
            body: List[ASTNode]


        @dataclass
        class ReturnNode(ASTNode):
            value: ASTNode


        @dataclass
        class FunctionCallNode(ASTNode):
            name: str
            args: List[ASTNode]


        # Parser class
        class Parser:
            def __init__(self, lexer: Lexer):
                self.lexer = lexer
                self.current_token = self.lexer.get_next_token()

            def error(self, message: str):
                raise SyntaxError(f"Parser error: {message} at line {self.current_token.line}")

            def eat(self, token_type: TokenType):
                if self.current_token.type == token_type:
                    self.current_token = self.lexer.get_next_token()
                else:
                    self.error(f"Expected token: {token_type}, Found: {self.current_token.type}")

            def program(self) -> List[ASTNode]:
                nodes = []
                while self.current_token.type != TokenType.EOF:
                    if self.current_token.type == TokenType.FUNCTION:
                        nodes.append(self.function_declaration())
                    else:
                        nodes.append(self.statement())
                return nodes

            def function_declaration(self) -> FunctionNode:
                self.eat(TokenType.FUNCTION)
                if self.current_token.type != TokenType.IDENTIFIER:
                    self.error("Expected function name")
                
                name = self.current_token.value
                self.eat(TokenType.IDENTIFIER)
                
                self.eat(TokenType.LPAREN)
                params = []
                while self.current_token.type == TokenType.IDENTIFIER:
                    params.append(self.current_token.value)
                    self.eat(TokenType.IDENTIFIER)
                    if self.current_token.type == TokenType.RPAREN:
                        break
                self.eat(TokenType.RPAREN)
                
                self.eat(TokenType.LBRACE)
                body = []
                while self.current_token.type != TokenType.RBRACE:
                    body.append(self.statement())
                self.eat(TokenType.RBRACE)
                
                return FunctionNode(name, params, body)

            def statement(self) -> ASTNode:
                if self.current_token.type == TokenType.IF:
                    return self.if_statement()
                elif self.current_token.type == TokenType.WHILE:
                    return self.while_statement()
                elif self.current_token.type == TokenType.RETURN:
                    return self.return_statement()
                elif self.current_token.type == TokenType.IDENTIFIER:
                    if self.lexer.current_char == '(':
                        return self.function_call()
                    return self.assignment_statement()
                else:
                    node = self.expr()
                    self.eat(TokenType.SEMICOLON)
                    return node

            def if_statement(self) -> IfNode:
                self.eat(TokenType.IF)
                self.eat(TokenType.LPAREN)
                condition = self.expr()
                self.eat(TokenType.RPAREN)
                
                self.eat(TokenType.LBRACE)
                body = []
                while self.current_token.type != TokenType.RBRACE:
                    body.append(self.statement())
                self.eat(TokenType.RBRACE)
                
                return IfNode(condition, body)

            def while_statement(self) -> WhileNode:
                self.eat(TokenType.WHILE)
                self.eat(TokenType.LPAREN)
                condition = self.expr()
                self.eat(TokenType.RPAREN)
                
                self.eat(TokenType.LBRACE)
                body = []
                while self.current_token.type != TokenType.RBRACE:
                    body.append(self.statement())
                self.eat(TokenType.RBRACE)
                
                return WhileNode(condition, body)

            def assignment_statement(self) -> AssignmentNode:
                name = self.current_token.value
                self.eat(TokenType.IDENTIFIER)
                self.eat(TokenType.EQUALS)
                value = self.expr()
                self.eat(TokenType.SEMICOLON)
                return AssignmentNode(name, value)

            def return_statement(self) -> ReturnNode:
                self.eat(TokenType.RETURN)
                value = self.expr()
                self.eat(TokenType.SEMICOLON)
                return ReturnNode(value)

            def function_call(self) -> FunctionCallNode:
                name = self.current_token.value
                self.eat(TokenType.IDENTIFIER)
                self.eat(TokenType.LPAREN)
                
                args = []
                while self.current_token.type != TokenType.RPAREN:
                    args.append(self.expr())
                    if self.current_token.type == TokenType.RPAREN:
                        break
                self.eat(TokenType.RPAREN)
                self.eat(TokenType.SEMICOLON)
                
                return FunctionCallNode(name, args)

            def expr(self) -> ASTNode:
                node = self.term()
                
                while self.current_token.type in (TokenType.PLUS, TokenType.MINUS):
                    token = self.current_token
                    if token.type == TokenType.PLUS:
                        self.eat(TokenType.PLUS)
                    else:
                        self.eat(TokenType.MINUS)
                    
                    node = BinOpNode(node, token.type, self.term())
                
                return node

            def term(self) -> ASTNode:
                node = self.factor()
                
                while self.current_token.type in (TokenType.MULTIPLY, TokenType.DIVIDE):
                    token = self.current_token
                    if token.type == TokenType.MULTIPLY:
                        self.eat(TokenType.MULTIPLY)
                    else:
                        self.eat(TokenType.DIVIDE)
                    
                    node = BinOpNode(node, token.type, self.factor())
                
                return node

            def factor(self) -> ASTNode:
                token = self.current_token
                
                if token.type == TokenType.NUMBER:
                    self.eat(TokenType.NUMBER)
                    return NumberNode(token.value)
                elif token.type == TokenType.IDENTIFIER:
                    self.eat(TokenType.IDENTIFIER)
                    return VariableNode(token.value)
                elif token.type == TokenType.LPAREN:
                    self.eat(TokenType.LPAREN)
                    node = self.expr()
                    self.eat(TokenType.RPAREN)
                    return node
                else:
                    self.error("Unexpected token in factor")


        # Semantic Analyzer class
        class SemanticAnalyzer:
            def __init__(self):
                self.variables: Dict[str, Any] = {}
                self.functions: Dict[str, FunctionNode] = {}

            def analyze(self, nodes: List[ASTNode]):
                for node in nodes:
                    self.visit(node)

            def visit(self, node: ASTNode):
                method_name = f'visit_{type(node).__name__}'
                visitor = getattr(self, method_name, self.generic_visit)
                return visitor(node)

            def visit_FunctionNode(self, node: FunctionNode):
                if node.name in self.functions:
                    raise ValueError(f"Function '{node.name}' has already been defined")
                self.functions[node.name] = node
                
                local_vars = self.variables.copy()
                for param in node.params:
                    local_vars[param] = None
                
                old_vars = self.variables
                self.variables = local_vars
                for stmt in node.body:
                    self.visit(stmt)
                self.variables = old_vars

            def visit_FunctionCallNode(self, node: FunctionCallNode):
                if node.name not in self.functions:
                    raise ValueError(f"Unknown function '{node.name}'")
                
                func = self.functions[node.name]
                if len(node.args) != len(func.params):
                    raise ValueError(f"Incorrect number of arguments for function '{node.name}'")

            def visit_AssignmentNode(self, node: AssignmentNode):
                self.variables[node.name] = None
                self.visit(node.value)

            def visit_VariableNode(self, node: VariableNode):
                if node.name not in self.variables:
                    raise ValueError(f"Unknown variable '{node.name}'")

            def generic_visit(self, node: ASTNode):
                pass


        # Code Generator class
        class CodeGenerator:
            def __init__(self):
                self.code = []
                self.indent_level = 0

            def indent(self):
                self.indent_level += 1

            def dedent(self):
                self.indent_level -= 1

            def add_line(self, line: str):
                self.code.append("    " * self.indent_level + line)

            def generate(self, nodes: List[ASTNode]) -> str:
                for node in nodes:
                    self.visit(node)
                return '\n'.join(self.code)

            def visit_NumberNode(self, node: NumberNode):
                self.add_line(f"result = {node.value}")

            def visit_BinOpNode(self, node: BinOpNode):
                self.visit(node.left)
                self.visit(node.right)
                if node.op == TokenType.PLUS:
                    self.add_line("result = left + right")
                elif node.op == TokenType.MINUS:
                    self.add_line("result = left - right")
                elif node.op == TokenType.MULTIPLY:
                    self.add_line("result = left * right")
                elif node.op == TokenType.DIVIDE:
                    self.add_line("result = left / right")

            def visit_AssignmentNode(self, node: AssignmentNode):
                self.visit(node.value)
                self.add_line(f"{node.name} = result")

            def visit_VariableNode(self, node: VariableNode):
                self.add_line(f"result = {node.name}")

            def visit_IfNode(self, node: IfNode):
                self.add_line("if condition:")
                self.indent()
                for stmt in node.body:
                    self.visit(stmt)
                self.dedent()

            def visit_WhileNode(self, node: WhileNode):
                self.add_line("while condition:")
                self.indent()
                for stmt in node.body:
                    self.visit(stmt)
                self.dedent()

            def visit_FunctionNode(self, node: FunctionNode):
                self.add_line(f"def {node.name}({', '.join(node.params)}):")
                self.indent()
                for stmt in node.body:
                    self.visit(stmt)
                self.dedent()

            def visit_ReturnNode(self, node: ReturnNode):
                self.visit(node.value)
                self.add_line("return result")

            def visit_FunctionCallNode(self, node: FunctionCallNode):
                self.add_line(f"result = {node.name}({', '.join([f'arg{i}' for i in range(len(node.args))])})")

            def generic_visit(self, node: ASTNode):
                pass


        # Main function
        def main():
            text = """
            function add(a, b) {
                return a + b;
            }

            function multiply(a, b) {
                return a * b;
            }

            function calculate() {
                var x = 5;
                var y = 10;
                var z = add(x, y);
                var result = multiply(z, 2);
                return result;
            }
            """

            lexer = Lexer(text)
            parser = Parser(lexer)
            semantic_analyzer = SemanticAnalyzer()
            code_generator = CodeGenerator()

            try:
                ast = parser.program()
                semantic_analyzer.analyze(ast)
                generated_code = code_generator.generate(ast)
                print(generated_code)
            except SyntaxError as e:
                print(f"Syntax error: {str(e)}")


        if __name__ == "__main__":
            main()
