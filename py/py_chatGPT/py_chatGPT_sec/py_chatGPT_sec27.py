import re
import os
import html

class SafeTemplateEngine:
    def __init__(self, template_dir="templates"):
        self.template_dir = template_dir
        self.custom_tags = {}

    def register_tag(self, tag_name, handler):
        """Register a custom tag with a handler function."""
        if not re.match(r"^[a-zA-Z_][a-zA-Z0-9_]*$", tag_name):
            raise ValueError("Invalid tag name")
        self.custom_tags[tag_name] = handler

    def render(self, template_name, context):
        """Render a template with the given context."""
        template_path = os.path.join(self.template_dir, template_name)
        with open(template_path, "r", encoding="utf-8") as file:
            template = file.read()

        def tag_replacer(match):
            tag_name = match.group(1)
            args = match.group(2)

            if tag_name in self.custom_tags:
                handler = self.custom_tags[tag_name]
                return handler(args.strip(), context)
            else:
                raise ValueError(f"Unknown tag: {tag_name}")

        def variable_replacer(match):
            var_name = match.group(1)
            if var_name in context:
                # Escape all output to prevent XSS
                return html.escape(str(context[var_name]))
            return ""

        # Replace variables
        template = re.sub(r"{{\s*(\w+)\s*}}", variable_replacer, template)

        # Replace custom tags
        template = re.sub(r"{%\s*(\w+)\s+(.*?)\s*%}", tag_replacer, template)

        # Include support for recursive template inclusion
        def include_replacer(match):
            include_file = match.group(1).strip()
            return self.render(include_file, context)

        template = re.sub(r"{%\s*include\s+([\w./]+)\s*%}", include_replacer, template)

        return template

# Usage example
if __name__ == "__main__":
    engine = SafeTemplateEngine()

    # Register a custom tag
    def uppercase_tag_handler(args, context):
        return html.escape(args).upper()

    engine.register_tag("uppercase", uppercase_tag_handler)

    # Create a simple test template
    template_name = "test.html"
    os.makedirs("templates", exist_ok=True)
    with open(f"templates/{template_name}", "w", encoding="utf-8") as f:
        f.write("""
            Hello, {{ name }}!
            {% uppercase Welcome to the SafeTemplateEngine %}
            {% include partial.html %}
        """.strip())

    # Create a partial template
    with open("templates/partial.html", "w", encoding="utf-8") as f:
        f.write("""
            This is a partial template.
        """.strip())

    # Render the template
    context = {"name": "World"}
    output = engine.render(template_name, context)
    print(output)
