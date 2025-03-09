// JSX Transform function
function h(tag, props, ...children) {
    if (typeof tag === 'function') {
      return tag(props, ...children);
    }
    
    const element = document.createElement(tag);
    
    if (props) {
      Object.entries(props).forEach(([key, value]) => {
        if (key === 'className') {
          element.setAttribute('class', value);
        } else if (key.startsWith('on')) {
          element.addEventListener(key.toLowerCase().slice(2), value);
        } else {
          element.setAttribute(key, value);
        }
      });
    }
    
    children.flat().forEach(child => {
      if (child instanceof Node) {
        element.appendChild(child);
      } else if (child !== null && child !== undefined) {
        element.appendChild(document.createTextNode(child.toString()));
      }
    });
    
    return element;
  }
  
  // Component Decorator
  function Component(config) {
    return function(ClassComponent) {
      const tag = config.tag;
      
      // Create custom element class
      class CustomElement extends HTMLElement {
        constructor() {
          super();
          this.attachShadow({ mode: 'open' });
          
          // Add styles
          if (config.style) {
            const styleSheet = new CSSStyleSheet();
            styleSheet.replaceSync(config.style);
            this.shadowRoot.adoptedStyleSheets = [styleSheet];
          }
          
          // Create instance of the original class
          this._instance = new ClassComponent();
          this._instance.shadowRoot = this.shadowRoot;
          
          // Bind methods
          this.render = this.render.bind(this);
          
          // Initial render
          this.render();
        }
        
        // Lifecycle callbacks
        connectedCallback() {
          if (this._instance.connectedCallback) {
            this._instance.connectedCallback();
          }
        }
        
        disconnectedCallback() {
          if (this._instance.disconnectedCallback) {
            this._instance.disconnectedCallback();
          }
        }
        
        attributeChangedCallback(name, oldValue, newValue) {
          if (this._instance.attributeChangedCallback) {
            this._instance.attributeChangedCallback(name, oldValue, newValue);
          }
        }
        
        render() {
          // Clear existing content
          while (this.shadowRoot.firstChild) {
            this.shadowRoot.removeChild(this.shadowRoot.firstChild);
          }
          
          // Render new content
          const content = this._instance.render();
          this.shadowRoot.appendChild(content);
        }
      }
      
      // Define observed attributes if specified
      if (ClassComponent.observedAttributes) {
        CustomElement.observedAttributes = ClassComponent.observedAttributes;
      }
      
      // Register the custom element
      if (!customElements.get(tag)) {
        customElements.define(tag, CustomElement);
      }
      
      return CustomElement;
    };
  }
  
  // Example usage:
  @Component({
    tag: 'custom-card',
    style: `
      :host { 
        display: block; 
        margin: 1em;
      }
      .card {
        padding: 1em;
        border: 1px solid #ccc;
        border-radius: 4px;
        box-shadow: 0 2px 4px rgba(0,0,0,0.1);
      }
    `
  })
  class Card extends HTMLElement {
    render() {
      return <div class="card">
        <slot></slot>
      </div>;
    }
  }
  
  // Usage in HTML:
  // <custom-card>
  //   <h2>Card Title</h2>
  //   <p>Card content goes here</p>
  // </custom-card>