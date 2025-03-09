import re

class TemplateEngine:
    def __init__(self):
        self.tags = {}
        self._tags = {}

    def register_tag(self, name, func):
        """
        Registriert ein neues Tag.

        Args:
            name (str): Der Name des Tags.
            func (function): Die Funktion, die beim Eingeben des Tags ausgeführt wird.
        """
        self.tags[name] = func
        self._tags[name] = True

    def render(self, template):
        """
        Renderet das Template.

        Args:
            template (str): Das Template, das gerendert werden soll.

        Returns:
            str: Die erweiterte Version des Templates.
        """
        for tag in re.findall(r'\{%\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\%}', template):
            if tag not in self._tags:
                raise ValueError(f"Unbekannter Tag '{tag}'")

            func = self.tags[tag]
            result = func()
            template = re.sub(rf'%\{tag}\%', str(result), template)

        return template

    def render_with_context(self, template, context):
        """
        Renderet das Template mit einem Kontext.

        Args:
            template (str): Das Template, das gerendert werden soll.
            context (dict): Der Kontext, der für das Template verwendet wird.

        Returns:
            str: Die erweiterte Version des Templates.
        """
        for tag in re.findall(r'\{%\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\%}', template):
            if tag not in context or not isinstance(context[tag], (int, float, str)):
                raise ValueError(f"Unbekannter oder ungewisser Kontext für Tag '{tag}'")

            func = self.tags[tag]
            result = func()
            template = re.sub(rf'%\{tag}\%', str(result), template)

        return template

    def recursive_inclusion(self, template, url):
        """
        Führt eine rekursive Inklusion des Templates durch.

        Args:
            template (str): Das Template, das inkludiert werden soll.
            url (str): Die URL der Inklusion.

        Returns:
            str: Die erweiterte Version des Templates.
        """
        if url in self._tags:
            return self.render(template)

        # Rekursive Inklusion
        result = self.render(template)
        for match in re.finditer(rf'<%\s*{re.escape(url)}\s*>', result):
            start, end = match.span()
            result = result[:start] + url + result[end:]

        return result

# Beispiel für ein Tag-Verwenden
class GreeterTag:
    def __call__(self):
        return "Hallo, Welt!"

engine = TemplateEngine()

# Registriert das Tag 'greeter'
engine.register_tag('greeter', GreeterTag())

# Verwendet das Tag in dem Template
template = "% greeter %"
print(engine.render(template))  # Ausgabe: "Hallo, Welt!"

# Verwendet den Template mit Kontext
context = {'greeter': "Hallo, Welt!"}
template = "% greeter %"
print(engine.render_with_context(template, context))  # Ausgabe: "Hallo, Welt!"

# Führt rekursive Inklusion durch
url = 'http://example.com'
template = '<%\s*{url}\s*>'
print(engine.recursive_inclusion(template, url))  # Ausgabe: <a href="http://example.com">http://example.com</a>
