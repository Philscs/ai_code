import re
import html
import os.path
from typing import Dict, Any, Optional
from dataclasses import dataclass
from abc import ABC, abstractmethod

class TemplateSecurityError(Exception):
    """Raised when a security violation is detected."""
    pass

class TemplateSyntaxError(Exception):
    """Raised when template syntax is invalid."""
    pass

@dataclass
class Token:
    type: str
    content: str
    line: int
    position: int

class AbstractNode(ABC):
    @abstractmethod
    def render(self, context: Dict[str, Any], engine: 'SecureTemplateEngine') -> str:
        pass

class TextNode(AbstractNode):
    def __init__(self, content: str):
        self.content = content
    
    def render(self, context: Dict[str, Any], engine: 'SecureTemplateEngine') -> str:
        return html.escape(self.content)

class VariableNode(AbstractNode):
    def __init__(self, var_name: str):
        self.var_name = var_name
    
    def render(self, context: Dict[str, Any], engine: 'SecureTemplateEngine') -> str:
        try:
            value = context.get(self.var_name, '')
            # Convert value to string and escape HTML
            return html.escape(str(value))
        except Exception as e:
            raise TemplateSecurityError(f"Error accessing variable {self.var_name}: {str(e)}")

class IncludeNode(AbstractNode):
    def __init__(self, template_name: str):
        self.template_name = template_name
        
    def render(self, context: Dict[str, Any], engine: 'SecureTemplateEngine') -> str:
        # Prevent directory traversal
        if '..' in self.template_name or self.template_name.startswith('/'):
            raise TemplateSecurityError("Invalid template path")
        
        # Set maximum recursion depth
        if engine.include_depth > 10:
            raise TemplateSecurityError("Maximum template inclusion depth exceeded")
            
        try:
            engine.include_depth += 1
            result = engine.render_template(self.template_name, context)
            engine.include_depth -= 1
            return result
        except Exception as e:
            engine.include_depth -= 1
            raise TemplateSecurityError(f"Error including template {self.template_name}: {str(e)}")

class CustomTagNode(AbstractNode):
    def __init__(self, tag_name: str, args: str):
        self.tag_name = tag_name
        self.args = args
    
    def render(self, context: Dict[str, Any], engine: 'SecureTemplateEngine') -> str:
        if self.tag_name not in engine.custom_tags:
            raise TemplateSecurityError(f"Unknown custom tag: {self.tag_name}")
        
        try:
            handler = engine.custom_tags[self.tag_name]
            result = handler(self.args, context)
            return html.escape(str(result))
        except Exception as e:
            raise TemplateSecurityError(f"Error in custom tag {self.tag_name}: {str(e)}")

class SecureTemplateEngine:
    def __init__(self, template_dir: str):
        self.template_dir = template_dir
        self.custom_tags = {}
        self.include_depth = 0
        
        # Compile regex patterns
        self.var_pattern = re.compile(r'\{\{\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\}\}')
        self.include_pattern = re.compile(r'\{%\s*include\s+"([^"]+)"\s*%\}')
        self.custom_tag_pattern = re.compile(r'\{%\s*([a-zA-Z_][a-zA-Z0-9_]*)\s+([^%]+)\s*%\}')
    
    def register_custom_tag(self, name: str, handler_func):
        """Register a custom tag handler function."""
        if not re.match(r'^[a-zA-Z_][a-zA-Z0-9_]*$', name):
            raise TemplateSecurityError("Invalid custom tag name")
        self.custom_tags[name] = handler_func
    
    def load_template(self, template_name: str) -> str:
        """Load template content with security checks."""
        # Prevent directory traversal
        if '..' in template_name or template_name.startswith('/'):
            raise TemplateSecurityError("Invalid template path")
            
        template_path = os.path.join(self.template_dir, template_name)
        if not os.path.exists(template_path):
            raise TemplateSecurityError(f"Template not found: {template_name}")
            
        try:
            with open(template_path, 'r', encoding='utf-8') as f:
                return f.read()
        except Exception as e:
            raise TemplateSecurityError(f"Error loading template {template_name}: {str(e)}")
    
    def parse(self, template_content: str) -> list[AbstractNode]:
        """Parse template content into nodes."""
        nodes = []
        position = 0
        line = 1
        
        while position < len(template_content):
            # Look for variable tags
            var_match = self.var_pattern.search(template_content, position)
            # Look for include tags
            include_match = self.include_pattern.search(template_content, position)
            # Look for custom tags
            custom_match = self.custom_tag_pattern.search(template_content, position)
            
            # Find the nearest match
            matches = [m for m in [var_match, include_match, custom_match] if m is not None]
            if not matches:
                # No more tags found, add remaining text
                text = template_content[position:]
                if text:
                    nodes.append(TextNode(text))
                break
                
            next_match = min(matches, key=lambda m: m.start())
            
            # Add text node for content before the match
            if next_match.start() > position:
                text = template_content[position:next_match.start()]
                nodes.append(TextNode(text))
                
            # Update line count
            line += template_content[position:next_match.start()].count('\n')
            
            # Handle the matched tag
            if next_match == var_match:
                var_name = next_match.group(1)
                nodes.append(VariableNode(var_name))
            elif next_match == include_match:
                template_name = next_match.group(1)
                nodes.append(IncludeNode(template_name))
            else:  # custom_match
                tag_name = next_match.group(1)
                args = next_match.group(2)
                nodes.append(CustomTagNode(tag_name, args))
            
            position = next_match.end()
            
        return nodes
    
    def render_template(self, template_name: str, context: Dict[str, Any]) -> str:
        """Render a template with the given context."""
        template_content = self.load_template(template_name)
        nodes = self.parse(template_content)
        
        result = []
        for node in nodes:
            try:
                result.append(node.render(context, self))
            except Exception as e:
                raise TemplateSecurityError(f"Error rendering template {template_name}: {str(e)}")
                
        return ''.join(result)

# Example usage:
if __name__ == "__main__":
    # Create template engine instance
    engine = SecureTemplateEngine("templates")
    
    # Register a custom tag
    def uppercase_tag(args: str, context: Dict[str, Any]) -> str:
        return args.strip().upper()
    
    engine.register_custom_tag("uppercase", uppercase_tag)
    
    # Example template content
    example_template = """
    <h1>Welcome, {{ username }}!</h1>
    {% uppercase "This text will be uppercase" %}
    {% include "footer.html" %}
    """
    
    # Example context
    context = {
        "username": "John Doe",
    }
    
    # Save example template
    with open("templates/example.html", "w") as f:
        f.write(example_template)
    
    # Render template
    try:
        result = engine.render_template("example.html", context)
        print(result)
    except (TemplateSecurityError, TemplateSyntaxError) as e:
        print(f"Error: {str(e)}")