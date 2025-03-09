// Base class for all components with Shadow DOM encapsulation
class BaseComponent extends HTMLElement {
    constructor() {
        super();
        // Create Shadow DOM
        this.attachShadow({ mode: 'closed' }); // Using closed mode for better encapsulation
    }

    // Security method to sanitize content
    sanitizeContent(content) {
        const template = document.createElement('template');
        template.innerHTML = content.trim();
        return template.content.cloneNode(true);
    }

    // Method to safely set styles
    setStyles(styles) {
        const styleSheet = new CSSStyleSheet();
        styleSheet.replaceSync(styles);
        this.shadowRoot.adoptedStyleSheets = [styleSheet];
    }

    // Method to safely render templates
    renderTemplate(template, data = {}) {
        let content = template;
        // Replace variables in template
        Object.entries(data).forEach(([key, value]) => {
            const regex = new RegExp(`{{\\s*${key}\\s*}}`, 'g');
            content = content.replace(regex, this.escapeHTML(value));
        });
        return this.sanitizeContent(content);
    }

    // Helper method to escape HTML
    escapeHTML(str) {
        const div = document.createElement('div');
        div.textContent = str;
        return div.innerHTML;
    }
}

// Template system
class TemplateSystem {
    static templates = new Map();

    static register(name, template) {
        if (typeof template !== 'string') {
            throw new Error('Template must be a string');
        }
        this.templates.set(name, template);
    }

    static get(name) {
        if (!this.templates.has(name)) {
            throw new Error(`Template "${name}" not found`);
        }
        return this.templates.get(name);
    }
}

// Example custom element using the base component
class CustomCard extends BaseComponent {
    constructor() {
        super();
        
        // Define styles with isolation
        const styles = `
            :host {
                display: block;
                padding: 16px;
                border-radius: 8px;
                box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            }
            .card-content {
                font-family: system-ui;
            }
            ::slotted(h2) {
                margin-top: 0;
                color: #2c3e50;
            }
        `;
        
        this.setStyles(styles);
    }

    // Lifecycle method
    connectedCallback() {
        const template = `
            <div class="card-content">
                <slot name="title"></slot>
                <slot></slot>
            </div>
        `;
        
        this.shadowRoot.appendChild(this.renderTemplate(template));
    }
}

// Example custom element with template system
class CustomList extends BaseComponent {
    constructor() {
        super();
        
        const styles = `
            :host {
                display: block;
            }
            ul {
                list-style: none;
                padding: 0;
            }
            li {
                padding: 8px;
                border-bottom: 1px solid #eee;
            }
        `;
        
        this.setStyles(styles);
    }

    // Observed attributes for reactive updates
    static get observedAttributes() {
        return ['items'];
    }

    // Attribute change handler
    attributeChangedCallback(name, oldValue, newValue) {
        if (name === 'items') {
            this.updateList(JSON.parse(newValue));
        }
    }

    updateList(items) {
        const template = TemplateSystem.get('list-item');
        const list = document.createElement('ul');
        
        items.forEach(item => {
            const li = document.createElement('li');
            li.appendChild(this.renderTemplate(template, { content: item }));
            list.appendChild(li);
        });

        // Clear and update content
        this.shadowRoot.innerHTML = '';
        this.shadowRoot.appendChild(list);
    }
}

// Register templates
TemplateSystem.register('list-item', '<div class="item">{{content}}</div>');

// Register custom elements
customElements.define('custom-card', CustomCard);
customElements.define('custom-list', CustomList);

// Export components for module usage
export {
    BaseComponent,
    TemplateSystem,
    CustomCard,
    CustomList
};
