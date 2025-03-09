from enum import Enum
from typing import List, Optional, Tuple, Union
import re

class TokenType(Enum):
    CHAR = "CHAR"
    DOT = "DOT"
    STAR = "STAR"
    PLUS = "PLUS"
    QUESTION = "QUESTION"
    LPAREN = "LPAREN"
    RPAREN = "RPAREN"
    LBRACKET = "LBRACKET"
    RBRACKET = "RBRACKET"
    PIPE = "PIPE"
    CARET = "CARET"
    DOLLAR = "DOLLAR"
    LOOKBEHIND = "LOOKBEHIND"
    LOOKAHEAD = "LOOKAHEAD"
    CONDITIONAL = "CONDITIONAL"

class Token:
    def __init__(self, type: TokenType, value: str = ""):
        self.type = type
        self.value = value

    def __str__(self):
        return f"Token({self.type}, '{self.value}')"

class RegexParser:
    def __init__(self):
        self.pos = 0
        self.tokens: List[Token] = []
        self.pattern = ""

    def tokenize(self, pattern: str) -> List[Token]:
        """Zerlegt den Regex-Pattern in Tokens."""
        self.pattern = pattern
        self.pos = 0
        self.tokens = []

        while self.pos < len(pattern):
            char = pattern[self.pos]
            
            if char == '.':
                self.tokens.append(Token(TokenType.DOT))
            elif char == '*':
                self.tokens.append(Token(TokenType.STAR))
            elif char == '+':
                self.tokens.append(Token(TokenType.PLUS))
            elif char == '?':
                self.tokens.append(Token(TokenType.QUESTION))
            elif char == '(':
                # Prüfe auf Lookbehind/Lookahead
                if self.peek_ahead("(?<="):
                    self.tokens.append(Token(TokenType.LOOKBEHIND))
                    self.pos += 3
                elif self.peek_ahead("(?="):
                    self.tokens.append(Token(TokenType.LOOKAHEAD))
                    self.pos += 2
                elif self.peek_ahead("(?("):
                    self.tokens.append(Token(TokenType.CONDITIONAL))
                    self.pos += 2
                else:
                    self.tokens.append(Token(TokenType.LPAREN))
            elif char == ')':
                self.tokens.append(Token(TokenType.RPAREN))
            elif char == '[':
                self.tokens.append(Token(TokenType.LBRACKET))
            elif char == ']':
                self.tokens.append(Token(TokenType.RBRACKET))
            elif char == '|':
                self.tokens.append(Token(TokenType.PIPE))
            elif char == '^':
                self.tokens.append(Token(TokenType.CARET))
            elif char == '$':
                self.tokens.append(Token(TokenType.DOLLAR))
            else:
                self.tokens.append(Token(TokenType.CHAR, char))
            
            self.pos += 1

        return self.tokens

    def peek_ahead(self, text: str) -> bool:
        """Prüft, ob der kommende Text mit dem gegebenen Text übereinstimmt."""
        if self.pos + len(text) > len(self.pattern):
            return False
        return self.pattern[self.pos:self.pos + len(text)] == text

    def parse(self, pattern: str) -> 'Node':
        """Parst den Regex-Pattern in einen Syntaxbaum."""
        self.tokens = self.tokenize(pattern)
        self.pos = 0
        return self.parse_expression()

    def parse_expression(self) -> 'Node':
        """Parst einen Regex-Ausdruck."""
        nodes = []
        
        while self.pos < len(self.tokens):
            token = self.tokens[self.pos]
            
            if token.type == TokenType.CHAR:
                nodes.append(CharNode(token.value))
            elif token.type == TokenType.DOT:
                nodes.append(DotNode())
            elif token.type == TokenType.LOOKBEHIND:
                nodes.append(self.parse_lookbehind())
            elif token.type == TokenType.LOOKAHEAD:
                nodes.append(self.parse_lookahead())
            elif token.type == TokenType.CONDITIONAL:
                nodes.append(self.parse_conditional())
            # ... weitere Token-Typen
            
            self.pos += 1
        
        if len(nodes) == 1:
            return nodes[0]
        return ConcatNode(nodes)

class Node:
    """Basisklasse für AST-Knoten."""
    pass

class CharNode(Node):
    def __init__(self, char: str):
        self.char = char

class DotNode(Node):
    pass

class LookbehindNode(Node):
    def __init__(self, expression: Node):
        self.expression = expression

class LookaheadNode(Node):
    def __init__(self, expression: Node):
        self.expression = expression

class ConditionalNode(Node):
    def __init__(self, condition: Node, true_branch: Node, false_branch: Optional[Node] = None):
        self.condition = condition
        self.true_branch = true_branch
        self.false_branch = false_branch

class ConcatNode(Node):
    def __init__(self, nodes: List[Node]):
        self.nodes = nodes

# Beispielverwendung:
if __name__ == "__main__":
    parser = RegexParser()
    # Beispiel für einen komplexen Regex mit Lookbehind und bedingtem Ausdruck
    pattern = "(?<=foo)bar(?(1)yes|no)"
    ast = parser.parse(pattern)
    
    # Hier könnte man den AST traversieren und den Regex auswerten
    # Dies würde zusätzliche Funktionalität zum Matching erfordern