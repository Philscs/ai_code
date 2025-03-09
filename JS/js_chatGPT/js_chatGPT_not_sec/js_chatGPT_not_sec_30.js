class TemplateEngine {
    constructor() {
      this.filters = {};
      this.partials = {};
      this.directives = {};
    }
  
    // Register a custom filter
    registerFilter(name, fn) {
      this.filters[name] = fn;
    }
  
    // Register a partial template
    registerPartial(name, template) {
      this.partials[name] = template;
    }
  
    // Register a custom directive
    registerDirective(name, fn) {
      this.directives[name] = fn;
    }
  
    // Render the template with the given context
    render(template, context) {
      const renderPartials = (template) => {
        return template.replace(/{{>\s*(\w+)\s*}}/g, (match, name) => {
          if (this.partials[name]) {
            return renderPartials(this.partials[name]);
          }
          return '';
        });
      };
  
      const applyFilters = (value, filters) => {
        return filters.reduce((currentValue, filter) => {
          const [filterName, ...args] = filter.split(':');
          if (this.filters[filterName]) {
            return this.filters[filterName](currentValue, ...args);
          }
          return currentValue;
        }, value);
      };
  
      const parseTemplate = (template) => {
        return template
          .replace(/{{#if\s+(.*?)}}([\s\S]*?){{\/if}}/g, (match, condition, content) => {
            return eval(condition) ? content : '';
          })
          .replace(/{{#each\s+(\w+)}}([\s\S]*?){{\/each}}/g, (match, iterable, content) => {
            if (context[iterable] && Array.isArray(context[iterable])) {
              return context[iterable]
                .map((item) => {
                  const subContext = { ...context, [iterable.slice(0, -1)]: item };
                  return this.render(content, subContext);
                })
                .join('');
            }
            return '';
          })
          .replace(/{{\s*([\w.]+)(\|[\w:.]+)*\s*}}/g, (match, variable, filterString) => {
            const filters = filterString ? filterString.split('|').slice(1) : [];
            const value = variable.split('.').reduce((acc, key) => acc[key], context);
            return applyFilters(value, filters);
          });
      };
  
      // Handle partial templates
      const renderedTemplate = renderPartials(template);
      return parseTemplate(renderedTemplate);
    }
  }
  
  // Example usage
  const engine = new TemplateEngine();
  
  // Register filters
  engine.registerFilter('uppercase', (value) => value.toUpperCase());
  engine.registerFilter('substring', (value, start, length) => value.substring(Number(start), Number(length)));
  
  // Register partials
  engine.registerPartial('header', '<h1>Welcome, {{ user | uppercase }}</h1>');
  
  // Register directives (example)
  engine.registerDirective('capitalize', (value) => value.charAt(0).toUpperCase() + value.slice(1));
  
  // Template
  const template = `
    {{> header }}
    {{#if user}}<p>User: {{ user | substring:0:5 }}</p>{{/if}}
    {{#each items}}<li>{{ item }}</li>{{/each}}
  `;
  
  // Context
  const context = {
    user: 'JohnDoe',
    items: ['Item1', 'Item2', 'Item3'],
  };
  
  // Render the template
  console.log(engine.render(template, context));
  