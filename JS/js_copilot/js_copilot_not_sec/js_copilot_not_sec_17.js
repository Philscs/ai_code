class Component {
  constructor(config) {
    this.tag = config.tag;
    this.style = config.style;
  }
}

function createElement(tag, props, ...children) {
  const element = document.createElement(tag);

  Object.keys(props).forEach((key) => {
    if (key === 'style') {
      Object.assign(element.style, props[key]);
    } else {
      element.setAttribute(key, props[key]);
    }
  });

  children.forEach((child) => {
    if (typeof child === 'string') {
      element.appendChild(document.createTextNode(child));
    } else {
      element.appendChild(child);
    }
  });

  return element;
}

function render(componentClass, container) {
  const component = new componentClass();
  const element = component.render();
  container.appendChild(element);
}

@Component({
  tag: 'custom-card',
  style: `
    .card {
      display: block;
    }
  `
})
class Card extends HTMLElement {
  render() {
    return createElement('div', { class: 'card' }, this.innerHTML);
  }
}

render(Card, document.getElementById('root'));
