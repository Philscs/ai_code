class VirtualDOMRenderer {
  constructor() {
    this.virtualDOM = null;
    this.realDOM = null;
    this.batchUpdates = false;
    this.updateQueue = [];
  }

  render(element, container) {
    this.virtualDOM = element;
    this.realDOM = this.createRealDOM(this.virtualDOM);
    container.appendChild(this.realDOM);
  }

  createRealDOM(virtualNode) {
    if (typeof virtualNode === 'string') {
      return document.createTextNode(virtualNode);
    }

    const { type, props, children } = virtualNode;
    const realNode = document.createElement(type);

    this.setProps(realNode, props);

    children.forEach(child => {
      const childNode = this.createRealDOM(child);
      realNode.appendChild(childNode);
    });

    return realNode;
  }

  setProps(element, props) {
    Object.keys(props).forEach(prop => {
      if (prop.startsWith('on')) {
        const eventName = prop.substring(2).toLowerCase();
        element.addEventListener(eventName, props[prop]);
      } else {
        element[prop] = props[prop];
      }
    });
  }

  updateProps(element, newProps, oldProps) {
    Object.keys(newProps).forEach(prop => {
      if (newProps[prop] !== oldProps[prop]) {
        if (prop.startsWith('on')) {
          const eventName = prop.substring(2).toLowerCase();
          element.removeEventListener(eventName, oldProps[prop]);
          element.addEventListener(eventName, newProps[prop]);
        } else {
          element[prop] = newProps[prop];
        }
      }
    });

    Object.keys(oldProps).forEach(prop => {
      if (!(prop in newProps)) {
        if (prop.startsWith('on')) {
          const eventName = prop.substring(2).toLowerCase();
          element.removeEventListener(eventName, oldProps[prop]);
        } else {
          element[prop] = null;
        }
      }
    });
  }

  diff(virtualNode, realNode) {
    if (typeof virtualNode === 'string') {
      if (virtualNode !== realNode.nodeValue) {
        realNode.nodeValue = virtualNode;
      }
      return;
    }

    if (virtualNode.type !== realNode.nodeName.toLowerCase()) {
      const newRealNode = this.createRealDOM(virtualNode);
      realNode.parentNode.replaceChild(newRealNode, realNode);
      return;
    }

    this.updateProps(realNode, virtualNode.props, realNode.props);

    const virtualChildren = virtualNode.children;
    const realChildren = Array.from(realNode.childNodes);

    const maxLen = Math.max(virtualChildren.length, realChildren.length);
    for (let i = 0; i < maxLen; i++) {
      this.diff(virtualChildren[i], realChildren[i]);
    }
  }

  scheduleUpdate(component) {
    if (this.batchUpdates) {
      this.updateQueue.push(component);
    } else {
      this.updateComponent(component);
    }
  }

  updateComponent(component) {
    const { props, state } = component;
    const newVirtualDOM = component.render();
    const oldVirtualDOM = component._virtualDOM;
    const oldRealDOM = component._realDOM;

    this.diff(newVirtualDOM, oldRealDOM);
    component._virtualDOM = newVirtualDOM;
    component._realDOM = oldRealDOM;
  }

  batchUpdate() {
    this.batchUpdates = true;
    this.updateQueue.forEach(component => {
      this.updateComponent(component);
    });
    this.batchUpdates = false;
    this.updateQueue = [];
  }
}

class Component {
  constructor(props) {
    this.props = props;
    this.state = {};
    this._virtualDOM = null;
    this._realDOM = null;
  }

  setState(newState) {
    this.state = { ...this.state, ...newState };
    this.renderer.scheduleUpdate(this);
  }

  render() {
    throw new Error('render method must be implemented');
  }

  componentDidMount() {}

  componentDidUpdate() {}

  componentWillUnmount() {}

  mount(container, renderer) {
    this.renderer = renderer;
    this._virtualDOM = this.render();
    this._realDOM = renderer.createRealDOM(this._virtualDOM);
    container.appendChild(this._realDOM);
    this.componentDidMount();
  }

  update() {
    const newVirtualDOM = this.render();
    renderer.diff(newVirtualDOM, this._realDOM);
    this._virtualDOM = newVirtualDOM;
    this.componentDidUpdate();
  }

  unmount() {
    this.componentWillUnmount();
    this._realDOM.parentNode.removeChild(this._realDOM);
  }
}

// Example usage
const renderer = new VirtualDOMRenderer();

class Counter extends Component {
  constructor(props) {
    super(props);
    this.state = { count: 0 };
  }

  increment() {
    this.setState({ count: this.state.count + 1 });
  }

  render() {
    return (
      <div>
        <h1>Counter: {this.state.count}</h1>
        <button onClick={() => this.increment()}>Increment</button>
      </div>
    );
  }
}

const counter = new Counter();
counter.mount(document.getElementById('app'), renderer);
