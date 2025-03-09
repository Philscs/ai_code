// Unique identifier generator for scoped styles
const generateHash = (str) => {
    let hash = 0;
    for (let i = 0; i < str.length; i++) {
      const char = str.charCodeAt(i);
      hash = ((hash << 5) - hash) + char;
      hash = hash & hash;
    }
    return Math.abs(hash).toString(36);
  };
  
  // Theme management
  class ThemeManager {
    constructor(defaultTheme = {}) {
      this.theme = defaultTheme;
      this.listeners = new Set();
    }
  
    setTheme(newTheme) {
      this.theme = { ...this.theme, ...newTheme };
      this.notifyListeners();
    }
  
    getTheme() {
      return this.theme;
    }
  
    subscribe(listener) {
      this.listeners.add(listener);
      return () => this.listeners.delete(listener);
    }
  
    notifyListeners() {
      this.listeners.forEach(listener => listener(this.theme));
    }
  }
  
  // Style sheet management
  class StyleSheet {
    constructor() {
      this.styles = new Map();
      this.injectedStyles = new Set();
      this.styleElement = null;
    }
  
    // Create styled component with scoped styles
    styled(tagName, cssTemplate, ...interpolations) {
      const hash = generateHash(cssTemplate.join(''));
      const className = `css-${hash}`;
  
      return (props) => {
        const resolvedStyles = this.resolveStyles(cssTemplate, interpolations, props);
        this.injectStyles(className, resolvedStyles);
  
        const element = document.createElement(tagName);
        element.className = className;
        return element;
      };
    }
  
    // Resolve dynamic styles and theme values
    resolveStyles(cssTemplate, interpolations, props) {
      return cssTemplate.reduce((acc, part, i) => {
        const interpolation = interpolations[i];
        const resolvedInterpolation = typeof interpolation === 'function'
          ? interpolation(props)
          : interpolation;
        return acc + part + (resolvedInterpolation || '');
      }, '');
    }
  
    // Style composition
    compose(...styles) {
      return (props) => {
        return styles.reduce((acc, style) => {
          if (typeof style === 'function') {
            return { ...acc, ...style(props) };
          }
          return { ...acc, ...style };
        }, {});
      };
    }
  
    // Runtime style injection
    injectStyles(className, styles) {
      if (this.injectedStyles.has(className)) return;
  
      if (!this.styleElement) {
        this.styleElement = document.createElement('style');
        document.head.appendChild(this.styleElement);
      }
  
      const scopedStyles = `.${className} { ${styles} }`;
      this.styleElement.textContent += scopedStyles;
      this.injectedStyles.add(className);
      this.styles.set(className, styles);
    }
  
    // Critical CSS extraction
    extractCriticalCSS() {
      const criticalStyles = [];
      for (const [className, styles] of this.styles) {
        if (document.querySelector(`.${className}`)) {
          criticalStyles.push(`.${className} { ${styles} }`);
        }
      }
      return criticalStyles.join('\n');
    }
  }
  
  // Usage example
  const themeManager = new ThemeManager({
    colors: {
      primary: '#007bff',
      secondary: '#6c757d'
    },
    spacing: {
      small: '8px',
      medium: '16px',
      large: '24px'
    }
  });
  
  const styleSheet = new StyleSheet();
  
  // Create a styled button
  const Button = styleSheet.styled('button', `
    padding: ${props => props.theme.spacing.medium};
    background-color: ${props => props.theme.colors.primary};
    color: white;
    border: none;
    border-radius: 4px;
    cursor: pointer;
  
    &:hover {
      opacity: 0.9;
    }
  `, props => props.customStyles);
  
  // Example of style composition
  const PrimaryButton = styleSheet.compose(
    Button,
    props => ({
      fontWeight: 'bold',
      textTransform: 'uppercase'
    })
  );
  
  // Export the engine
  export {
    ThemeManager,
    StyleSheet,
    generateHash
  };