import re
from collections import defaultdict

# Token-Typen
TOKEN_TYPES = {
    'LET': r'\blet\b',
    'PRINT': r'\bprint\b',
    'IF': r'\bif\b',
    'WHILE': r'\bwhile\b',
    'FUNC': r'\bfunc\b'
}

# Operator-Typen
OPERATORS = {
    'ADD': '+',
    'SUB': '-',
    'MUL': '*',
    'DIV': '/'
}

class Token:
    def __init__(self, type, value):
        self.type = type
        self.value = value

class Lexer:
    def __init__(self, source_code):
        self.source_code = source_code
        self.pos = 0
        self.current_token = None

    def lex(self):
        tokens = []
        while self.pos < len(self.source_code):
            for token_type, regex in TOKEN_TYPES.items():
                match = re.match(regex, self.source_code[self.pos:])
                if match:
                    start_pos = self.pos
                    value = self.source_code[start_pos:start_pos + len(match.group(0))]
                    tokens.append(Token(token_type, value))
                    self.pos += len(value)
                    break
            else:
                raise Exception("Invalid token")

        return tokens

class Parser:
    def __init__(self, lexer):
        self.lexer = lexer
        self.tokens = self.lexer.lex()

    def parse(self):
        # Deklamationen bearbeiten
        decls = []
        while self.tokens and self.tokens[0].type == 'LET':
            decls.append(self.parse_decl())

        # Ausdrücke bearbeiten
        expressions = []
        i = 0
        while i < len(self.tokens):
            if self.tokens[i].type == 'PRINT' or self.tokens[i].type == 'FUNC':
                expressions.append(self.parse_expr())
            else:
                i += 1

        # Bedingungen und Schleifen bearbeiten
        conditionals = []
        while self.tokens and self.tokens[0].type == 'IF':
            conditionals.append(self.parse_conditional())

        return decls, expressions, conditionals

    def parse_decl(self):
        token = self.tokens.pop(0)
        if token.type == 'LET':
            name = token.value
            value = self.parse_expr()
            return f'{name} = {value}'
        else:
            raise Exception("Invalid declaration")

    def parse_expr(self):
        token = self.tokens.pop(0)
        value = token.value

        # Ausdrücke untereinander kombinieren
        while self.tokens and (self.tokens[0].type in ['ADD', 'SUB'] or self.tokens[0].type == 
'MUL' or self.tokens[0].type == 'DIV'):
            operator = self.tokens.pop(0)
            right_expr = self.parse_expr()
            value = f"({value} {operator.value} {right_expr})"

        return value

    def parse_conditional(self):
        token = self.tokens.pop(0)

        # Bedingung definieren
        condition = self.parse_expr()

        if self.tokens[0].type == 'IF':
            self.tokens.pop(0)  # IF Token entfernen
            consequence = self.parse_block()
            return f'{condition} {token.value} {{\n{consequence}\n}}'

        elif self.tokens[0].type == 'WHILE':
            self.tokens.pop(0)  # WHILE Token entfernen
            condition = self.parse_expr()
            self.tokens.append(Token('IF', None))  # IF Token wieder hinzufügen, um Bedingung zu definieren
            consequence = self.parse_block()

        return f'while {condition} {{\n{consequence}\n}}'

    def parse_block(self):
        block = ""
        while self.tokens and (self.tokens[0].type in ['ADD', 'SUB'] or self.tokens[0].type == 
'MUL' or self.tokens[0].type == 'DIV'):
            operator = self.tokens.pop(0)
            right_expr = self.parse_expr()
            block += f'{operator.value} {right_expr}\n'

        return block

class Semantiker:
    def __init__(self, parser):
        self.parser = parser
        self.Variables = {}

    def semantisch_analyse(self):
        decls, expressions, conditionals = self.parser.parse()

        for decl in decls:
            # Deklamationen analysieren
            name = decl.split(' = ')[0]
            if name in self.Variables:
                raise Exception(f'Doppelte Deklaration von {name}')
            else:
                self.Variables[name] = None

        for expr in expressions:
            # Ausdrücke analysieren
            value = self.parse_expr(expr)
            if expr != "x":
                self.Variables["x"] = value

        for cond in conditionals:
            # Bedingungen analysieren
            condition = cond.split("if")[1].split("{")[0]
            consequence = cond.split("}")[1]

            self.semantisch_analyse_condition(condition, consequence)

    def semantisch_analyse_condition(self, condition, consequence):
        # Bedingungsbedeutung analysieren
        if condition in self.Variables:
            value = self.Variables[condition]
        else:
            raise Exception(f'Unbekannte Variable: {condition}')

        for line in consequence.split('\n'):
            line = line.strip()

            # Ausdrücke untereinander kombinieren
            while ',' in line and (line[line.find(',') - 1] in OPERATORS or line[line.find(',') + 
1] in OPERATORS):
                operator = None
                if line[line.find(',') - 1] in OPERATORS:
                    operator = line[line.find(',') - 1]
                else:
                    operator = line[line.find(',') + 1]

                left_value = self.Variables.get(line[:line.find(',')], None)
                right_value = self.Variables.get(line[line.find(',') + 2:], None)

                if isinstance(left_value, list):
                    left_value = f"[{', '.join(map(str, left_value))}]"
                else:
                    left_value = str(left_value)

                if isinstance(right_value, list):
                    right_value = f"[{', '.join(map(str, right_value))}]"
                else:
                    right_value = str(right_value)

                line = f"({left_value} {operator} {right_value})"

            value = self.eval_expr(value, line.strip())

        self.Variables[condition] = value

    def parse_expr(self, expr):
        # Ausdrücke auswerten
        return eval(expr)

    def eval_expr(self, left_value, right_value):
        # Werte kombinieren
        if isinstance(left_value, list) and isinstance(right_value, list):
            # Zahlenkombination
            return [self.eval_expr(l, r) for l, r in zip(left_value, right_value)]
        else:
            return eval(f"({left_value} {OPERATORS[right_value]} {right_value})")

class CodeGenerator:
    def __init__(self, semantiker):
        self.semantiker = semantiker

    def generiere_code(self):
        decls, expressions, conditionals = self.semantiker.semantisch_analyse()

        # Deklamationen generieren
        code_decls = ""
        for decl in decls:
            if decl.split(' = ')[0] == 'x':
                pass
            else:
                code_decls += f'var {decl} = 0;\n'

        # Ausdrücke generieren
        code_expressions = ""
        for expr in expressions:
            if expr != "x":
                code_expressions += f'set x = {expr};\n'
            else:
                pass

        # Bedingungen generieren
        code_conditionals = ""
        for cond in conditionals:
            code_conditionals += f'if ({cond}) {{\n'

        return code_decls + "\n" + code_expressions + "\n" + code_conditionals
