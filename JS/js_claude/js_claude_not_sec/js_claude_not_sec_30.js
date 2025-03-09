class TemplateEngine {
    constructor() {
        this.directives = new Map();
        this.filters = new Map();
        this.partials = new Map();

        // Register default directives
        this.registerDirective('if', this._handleIf.bind(this));
        this.registerDirective('for', this._handleFor.bind(this));
        
        // Register default filters
        this.registerFilter('uppercase', (value) => String(value).toUpperCase());
        this.registerFilter('lowercase', (value) => String(value).toLowerCase());
        this.registerFilter('capitalize', (value) => {
            const str = String(value);
            return str.charAt(0).toUpperCase() + str.slice(1);
        });
    }

    // Public API
    registerDirective(name, handler) {
        this.directives.set(name, handler);
    }

    registerFilter(name, handler) {
        this.filters.set(name, handler);
    }

    registerPartial(name, template) {
        this.partials.set(name, template);
    }

    render(template, context) {
        // Replace partials
        template = this._processPartials(template);
        
        // Process directives
        template = this._processDirectives(template, context);
        
        // Process filters
        template = this._processFilters(template, context);
        
        // Replace variables
        template = this._processVariables(template, context);
        
        return template;
    }

    // Private methods
    _processDirectives(template, context) {
        const directiveRegex = /{{\s*([\w]+)\s+([^}]+)}}/g;
        
        return template.replace(directiveRegex, (match, directive, content) => {
            const handler = this.directives.get(directive);
            if (handler) {
                return handler(content.trim(), context);
            }
            return match;
        });
    }

    _processFilters(template, context) {
        const filterRegex = /{{\s*([^|]+)\s*\|\s*([\w]+)\s*}}/g;
        
        return template.replace(filterRegex, (match, content, filter) => {
            const filterFn = this.filters.get(filter);
            if (filterFn) {
                const value = this._evaluateExpression(content.trim(), context);
                return filterFn(value);
            }
            return match;
        });
    }

    _processVariables(template, context) {
        const variableRegex = /{{\s*([^}]+)\s*}}/g;
        
        return template.replace(variableRegex, (match, path) => {
            return this._evaluateExpression(path.trim(), context);
        });
    }

    _processPartials(template) {
        const partialRegex = /{{\s*>\s*([\w]+)\s*}}/g;
        
        return template.replace(partialRegex, (match, partialName) => {
            const partial = this.partials.get(partialName);
            return partial || match;
        });
    }

    _evaluateExpression(expression, context) {
        try {
            const paths = expression.split('.');
            let result = context;
            
            for (const path of paths) {
                result = result[path];
                if (result === undefined) break;
            }
            
            return result !== undefined ? result : '';
        } catch (error) {
            return '';
        }
    }

    _handleIf(content, context) {
        const [condition, ...body] = content.split(',').map(part => part.trim());
        const result = this._evaluateExpression(condition, context);
        
        if (result) {
            return this.render(body.join(','), context);
        }
        return '';
    }

    _handleFor(content, context) {
        const [iteratorDef, ...body] = content.split(',').map(part => part.trim());
        const [item, collection] = iteratorDef.split(' in ').map(part => part.trim());
        
        const items = this._evaluateExpression(collection, context);
        if (!Array.isArray(items)) return '';
        
        return items.map(itemValue => {
            const itemContext = { ...context, [item]: itemValue };
            return this.render(body.join(','), itemContext);
        }).join('');
    }
}

// Usage Example:
const engine = new TemplateEngine();

// Register a custom directive
engine.registerDirective('uppercase', (content, context) => {
    return content.toUpperCase();
});

// Register a custom filter
engine.registerFilter('reverse', (value) => {
    return String(value).split('').reverse().join('');
});

// Register a partial template
engine.registerPartial('header', '<header>{{ title }}</header>');

// Template with various features
const template = `
    {{ > header }}
    <div>
        {{ if user.isAdmin, 
            <h1>Welcome Admin: {{ user.name | capitalize }}</h1>
        }}
        
        <ul>
        {{ for item in items, 
            <li>{{ item.name | uppercase }} - {{ item.price }}</li>
        }}
        </ul>
        
        {{ customText | reverse }}
    </div>
`;

// Context data
const context = {
    title: 'My Shop',
    user: {
        name: 'john',
        isAdmin: true
    },
    items: [
        { name: 'Item 1', price: 10 },
        { name: 'Item 2', price: 20 },
        { name: 'Item 3', price: 30 }
    ],
    customText: 'Hello World!'
};

// Render the template
console.log(engine.render(template, context));