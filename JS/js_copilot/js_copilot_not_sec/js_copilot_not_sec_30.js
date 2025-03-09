class TemplateEngine {
  constructor() {
    this.templates = {};
    this.directives = {};
    this.filters = {};
  }

  registerTemplate(name, template) {
    this.templates[name] = template;
  }

  registerDirective(name, directive) {
    this.directives[name] = directive;
  }

  registerFilter(name, filter) {
    this.filters[name] = filter;
  }

  render(templateName, data) {
    const template = this.templates[templateName];
    if (!template) {
      throw new Error(`Template '${templateName}' not found.`);
    }

    const renderedTemplate = this._renderTemplate(template, data);
    return renderedTemplate;
  }

  _renderTemplate(template, data) {
    let rendered = '';

    for (const node of template) {
      if (typeof node === 'string') {
        rendered += node;
      } else if (node.type === 'conditional') {
        const condition = this._evaluateExpression(node.condition, data);
        if (condition) {
          rendered += this._renderTemplate(node.content, data);
        }
      } else if (node.type === 'loop') {
        const list = this._evaluateExpression(node.list, data);
        for (const item of list) {
          const itemData = { ...data, [node.variable]: item };
          rendered += this._renderTemplate(node.content, itemData);
        }
      } else if (node.type === 'directive') {
        const directive = this.directives[node.name];
        if (directive) {
          rendered += directive(node, data);
        }
      }
    }

    return rendered;
  }

  _evaluateExpression(expression, data) {
    // Evaluate the expression using the provided data
    // You can use a library like 'lodash' for this
    // Example: return _.get(data, expression);
  }
}

// Usage example:

const engine = new TemplateEngine();

// Register a template
engine.registerTemplate('myTemplate', [
  'Hello, ',
  { type: 'conditional', condition: 'user.loggedIn', content: ['{{ user.name }}'] },
  { type: 'loop', list: 'items', variable: 'item', content: ['<li>{{ item }}</li>'] }
]);

// Register a custom directive
engine.registerDirective('uppercase', (node, data) => {
  const content = engine._renderTemplate(node.content, data);
  return content.toUpperCase();
});

// Register a custom filter
engine.registerFilter('capitalize', (value) => {
  return value.charAt(0).toUpperCase() + value.slice(1);
});

// Render the template
const data = {
  user: {
    loggedIn: true,
    name: 'John Doe'
  },
  items: ['Apple', 'Banana', 'Orange']
};

const renderedTemplate = engine.render('myTemplate', data);
console.log(renderedTemplate);
