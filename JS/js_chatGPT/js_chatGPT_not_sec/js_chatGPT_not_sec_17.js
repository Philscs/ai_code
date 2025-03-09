// Importiere Babel für die JSX-Verarbeitung
import { transform } from '@babel/standalone';

function Component(config) {
  return function (constructor) {
    // Registriere das Web Component
    customElements.define(
      config.tag,
      class extends constructor {
        constructor() {
          super();
          const shadow = this.attachShadow({ mode: 'open' });

          if (config.style) {
            const style = document.createElement('style');
            style.textContent = config.style;
            shadow.appendChild(style);
          }
        }

        connectedCallback() {
          this.update();
        }

        update() {
          const shadow = this.shadowRoot;
          if (shadow) {
            shadow.innerHTML = shadow.querySelector('style')?.outerHTML || '';
            const jsxElement = this.render();
            if (jsxElement) {
              shadow.appendChild(parseJSX(jsxElement));
            }
          }
        }

        render() {
          return null;
        }
      }
    );
  };
}

function parseJSX(jsxElement) {
  if (typeof jsxElement === 'string') {
    return document.createTextNode(jsxElement);
  }

  const { type, props } = jsxElement;
  const element = document.createElement(type);

  if (props) {
    for (const [key, value] of Object.entries(props)) {
      if (key === 'children') {
        value.forEach((child) => element.appendChild(parseJSX(child)));
      } else if (key.startsWith('on')) {
        const event = key.substring(2).toLowerCase();
        element.addEventListener(event, value);
      } else {
        element.setAttribute(key, value);
      }
    }
  }

  return element;
}

/** Beispiel für die Nutzung der @Component-Dekoration */
@Component({
  tag: 'custom-card',
  style: `
    :host {
      display: block;
      border: 1px solid #ccc;
      padding: 16px;
      border-radius: 8px;
    }
    .card {
      background: #fff;
    }
  `,
})
class Card extends HTMLElement {
  render() {
    return {
      type: 'div',
      props: {
        class: 'card',
        children: [
          { type: 'slot', props: {} },
        ],
      },
    };
  }
}

// Fügt die Web Component in den DOM ein
document.body.innerHTML = `
  <custom-card>
    <p>Inhalt im Slot</p>
  </custom-card>
`;

// JSX sollte von Babel oder einem ähnlichen Tool transpiliert werden,
// wenn die Syntax direkt in JavaScript genutzt werden soll.
