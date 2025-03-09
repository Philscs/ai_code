import re

class RegexParser:
    def __init__(self, pattern, flags=0):
        """Initialisiert den Parser mit einem regulären Ausdruck und optionalen Flags."""
        self.pattern = pattern
        self.flags = flags
        self.compiled_pattern = None

    def compile(self):
        """Kompiliert das Muster für die spätere Verwendung."""
        try:
            self.compiled_pattern = re.compile(self.pattern, self.flags)
        except re.error as e:
            raise ValueError(f"Fehler beim Kompilieren des Musters: {e}")

    def parse(self, text):
        """Parst den gegebenen Text und gibt alle Treffer zurück."""
        if not self.compiled_pattern:
            self.compile()
        return self.compiled_pattern.finditer(text)

    def supports_lookaround(self):
        """Überprüft, ob Lookahead und Lookbehind verwendet werden können."""
        lookaround_pattern = r'(?<=foo)bar|bar(?=baz)'
        try:
            re.compile(lookaround_pattern)
            return True
        except re.error:
            return False

    def supports_conditional(self):
        """Überprüft, ob bedingte Ausdrücke verwendet werden können."""
        conditional_pattern = r'(?(?=foo)bar|baz)'
        try:
            re.compile(conditional_pattern)
            return True
        except re.error:
            return False

# Beispiel
if __name__ == "__main__":
    # Regulärer Ausdruck mit Lookbehind und Lookahead
    pattern = r'(?<=foo)bar(?=baz)'
    text = "foobarbaz foobarqux"

    parser = RegexParser(pattern)
    
    print("Unterstützt Lookaround:", parser.supports_lookaround())
    print("Unterstützt bedingte Ausdrücke:", parser.supports_conditional())

    try:
        matches = parser.parse(text)
        for match in matches:
            print("Gefunden:", match.group())
    except ValueError as e:
        print(e)
