// A basic CSS-in-JS engine
class CSSInJSEngine {
    constructor() {
        this.styleSheet = document.createElement('style');
        document.head.appendChild(this.styleSheet);
        this.styles = new Map(); // For critical CSS extraction
    }

    // Generates a unique scope for styles
    generateScope() {
        return `css-${Math.random().toString(36).substring(2, 10)}`;
    }

    // Inject styles into the DOM at runtime
    injectStyles(scope, styles) {
        const scopedStyles = Object.entries(styles)
            .map(([selector, rules]) => {
                const cssRules = Object.entries(rules)
                    .map(([prop, value]) => `${prop}: ${value};`)
                    .join(' ');
                return `.${scope} ${selector} { ${cssRules} }`;
            })
            .join(' ');

        this.styles.set(scope, scopedStyles);
        this.updateStyleSheet();
    }

    // Update the style sheet in the DOM
    updateStyleSheet() {
        this.styleSheet.innerHTML = Array.from(this.styles.values()).join(' ');
    }

    // Scoped Styles
    createScopedStyles(styles) {
        const scope = this.generateScope();
        this.injectStyles(scope, styles);
        return scope;
    }

    // Dynamic Theming
    applyTheme(scope, theme) {
        if (!this.styles.has(scope)) {
            throw new Error('Scope not found');
        }
        const updatedStyles = Object.entries(theme).reduce((acc, [selector, rules]) => {
            acc[selector] = { ...rules };
            return acc;
        }, {});
        this.injectStyles(scope, updatedStyles);
    }

    // Style Composition
    composeStyles(...styles) {
        return styles.reduce((acc, style) => ({ ...acc, ...style }), {});
    }

    // Critical CSS Extraction
    extractCriticalCSS() {
        return Array.from(this.styles.values()).join(' ');
    }
}

// Usage Example
const engine = new CSSInJSEngine();

// Scoped styles
const buttonScope = engine.createScopedStyles({
    button: {
        background: 'blue',
        color: 'white',
        padding: '10px 20px',
        border: 'none',
        borderRadius: '5px',
    },
});

const containerScope = engine.createScopedStyles({
    div: {
        margin: '20px',
        padding: '10px',
        background: 'lightgray',
    },
});

// Apply dynamic theming
engine.applyTheme(buttonScope, {
    button: {
        background: 'green',
    },
});

// Compose styles
const composedStyles = engine.composeStyles(
    { button: { fontSize: '14px' } },
    { button: { fontWeight: 'bold' } }
);
engine.injectStyles(buttonScope, composedStyles);

// Extract critical CSS
console.log('Critical CSS:', engine.extractCriticalCSS());

// The resulting scoped styles can be applied to HTML elements like so:
// <button class="css-unique-scope">Click Me</button>
// <div class="css-unique-scope">Content</div>
