from enum import Enum, auto
from dataclasses import dataclass
from typing import List, Dict, Any, Optional
import ast

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
            'return': TokenType.RETURN
        }
        token_type = keywords.get(result, TokenType.IDENTIFIER)
        return Token(token_type, result, self.line, self.column)

    def get_next_token(self) -> Token:
        while self.current_char:
            if self.current_char.isspace():
                self.skip_whitespace()
                continue

            if self.current_char.isdigit():
                return self.number()

            if self.current_char.isalpha():
                return self.identifier()

            # Einzelne Zeichen
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
                token = Token(char_to_token[self.current_char], self.current_char, 
                            self.line, self.column)
                self.advance()
                return token

            raise SyntaxError(f"Unerwartetes Zeichen '{self.current_char}' "
                            f"in Zeile {self.line}, Spalte {self.column}")

        return Token(TokenType.EOF, '', self.line, self.column)

# AST-Knoten
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

# Parser
class Parser:
    def __init__(self, lexer: Lexer):
        self.lexer = lexer
        self.current_token = self.lexer.get_next_token()

    def error(self, message: str):
        raise SyntaxError(f"Parser-Fehler: {message} in Zeile {self.current_token.line}")

    def eat(self, token_type: TokenType):
        if self.current_token.type == token_type:
            self.current_token = self.lexer.get_next_token()
        else:
            self.error(f"Erwarteter Token: {token_type}, Gefunden: {self.current_token.type}")

    def program(self) -> List[ASTNode]:
        """Haupteinstiegspunkt des Parsers"""
        nodes = []
        while self.current_token.type != TokenType.EOF:
            if self.current_token.type == TokenType.FUNCTION:
                nodes.append(self.function_declaration())
            else:
                nodes.append(self.statement())
        return nodes

    def function_declaration(self) -> FunctionNode:
        """Parst eine Funktionsdeklaration"""
        self.eat(TokenType.FUNCTION)
        if self.current_token.type != TokenType.IDENTIFIER:
            self.error("Funktionsname erwartet")
        
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
        """Parst ein einzelnes Statement"""
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
        """Parst eine if-Anweisung"""
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
        """Parst eine while-Schleife"""
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
        """Parst eine Zuweisung"""
        name = self.current_token.value
        self.eat(TokenType.IDENTIFIER)
        self.eat(TokenType.EQUALS)
        value = self.expr()
        self.eat(TokenType.SEMICOLON)
        return AssignmentNode(name, value)

    def return_statement(self) -> ReturnNode:
        """Parst eine return-Anweisung"""
        self.eat(TokenType.RETURN)
        value = self.expr()
        self.eat(TokenType.SEMICOLON)
        return ReturnNode(value)

    def function_call(self) -> FunctionCallNode:
        """Parst einen Funktionsaufruf"""
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
        """Parst einen arithmetischen Ausdruck"""
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
        """Parst einen Term (Multiplikation/Division)"""
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
        """Parst einen Faktor"""
        token = self.current_token
        
        if token.type == TokenType.NUMBER:
            self.eat(TokenType.NUMBER)
            return NumberNode(int(token.value))
        elif token.type == TokenType.IDENTIFIER:
            self.eat(TokenType.IDENTIFIER)
            return VariableNode(token.value)
        elif token.type == TokenType.LPAREN:
            self.eat(TokenType.LPAREN)
            node = self.expr()
            self.eat(TokenType.RPAREN)
            return node
        else:
            self.error("Unerwarteter Token im Faktor")

# Semantische Analyse
class SemanticAnalyzer:
    def __init__(self):
        self.variables: Dict[str, Any] = {}
        self.functions: Dict[str, FunctionNode] = {}

    def analyze(self, nodes: List[ASTNode]):
        """Führt die semantische Analyse durch"""
        for node in nodes:
            self.visit(node)

    def visit(self, node: ASTNode):
        """Besucht einen AST-Knoten und prüft semantische Regeln"""
        method_name = f'visit_{type(node).__name__}'
        visitor = getattr(self, method_name, self.generic_visit)
        return visitor(node)

    def visit_FunctionNode(self, node: FunctionNode):
        """Prüft Funktionsdeklarationen"""
        if node.name in self.functions:
            raise ValueError(f"Funktion '{node.name}' wurde bereits definiert")
        self.functions[node.name] = node
        
        # Lokalen Scope für Parameter erstellen
        local_vars = self.variables.copy()
        for param in node.params:
            local_vars[param] = None
        
        # Body mit lokalem Scope analysieren
        old_vars = self.variables
        self.variables = local_vars
        for stmt in node.body:
            self.visit(stmt)
        self.variables = old_vars

    def visit_FunctionCallNode(self, node: FunctionCallNode):
        """Prüft Funktionsaufrufe"""
        if node.name not in self.functions:
            raise ValueError(f"Unbekannte Funktion '{node.name}'")
        
        func = self.functions[node.name]
        if len(node.args) != len(func.params):
            raise ValueError(f"Falsche Anzahl von Argumenten für Funktion '{node.name}'")

    def visit_AssignmentNode(self, node: AssignmentNode):
        """Prüft Variablenzuweisungen"""
        self.variables[node.name] = None
        self.visit(node.value)

    def visit_VariableNode(self, node: VariableNode):
        """Prüft Variablenverwendung"""
        if node.name not in self.variables:
            raise ValueError(f"Unbekannte Variable '{node.name}'")

    def generic_visit(self, node: ASTNode):
        """Standard-Visitor für andere Knotentypen"""
        pass

# Python-Codegenerator
class CodeGenerator:
    def __init__(self):
        self.code = []
        self.indent_level = 0

    def indent(self):
        """Erhöht die Einrückung"""
        self.indent_level += 1

    def dedent(self):
        """Verringert die Einrückung"""
        self.indent_level -= 1

    def add_line(self, line: str):
        """Fügt eine Codezeile mit korrekter Einrückung hinzu"""
        self.code.append("    " * self.indent_level + line)