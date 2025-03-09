from enum import Enum
from typing import List, Optional

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
        self.tokens: List[Token] = []

    def tokenize(self, pattern: str) -> List[Token]:
        """Tokenizes the regex pattern."""
        self.tokens = []
        pos = 0
        while pos < len(pattern):
            char = pattern[pos]
            if char == '.':
                self.tokens.append(Token(TokenType.DOT))
            elif char in ['*', '+', '?', '(', ')', '[', ']', '|', '^', '$']:
                self.tokens.append(Token(TokenType(char)))
            else:
                self.tokens.append(Token(TokenType.CHAR, char))
            pos += 1
        return self.tokens

    def parse(self, pattern: str) -> 'Node':
        """Parses the regex pattern into an AST."""
        self.tokens = self.tokenize(pattern)
        return self.parse_expression()

    def parse_expression(self) -> 'Node':
        """Parses a regex expression."""
        nodes = []
        pos = 0
        while pos < len(self.tokens):
            token = self.tokens[pos]
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
            pos += 1
        return nodes[0] if len(nodes) == 1 else ConcatNode(nodes)

class Node:
    """Base class for AST nodes."""
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

# Example usage:
if __name__ == "__main__":
    parser = RegexParser()
    pattern = "(?<=foo)bar(?(1)yes|no)"
    ast = parser.parse(pattern)
