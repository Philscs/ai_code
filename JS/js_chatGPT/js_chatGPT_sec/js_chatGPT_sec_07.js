class VNode {
    constructor(tag, props = {}, children = []) {
      this.tag = tag;
      this.props = props;
      this.children = children;
      this.dom = null; // Reference to the real DOM node
    }
  }
  
  class VirtualDOM {
    constructor() {
      this.rootVNode = null;
      this.rootDOM = null;
      this.batchQueue = [];
      this.batchScheduled = false;
    }
  
    createElement(tag, props, ...children) {
      const flatChildren = children.flat().map(child =>
        typeof child === 'string' ? new VNode(null, null, child) : child
      );
      return new VNode(tag, props, flatChildren);
    }
  
    mount(vnode, container) {
      const dom = this.createRealNode(vnode);
      container.appendChild(dom);
      vnode.dom = dom;
      this.rootVNode = vnode;
      this.rootDOM = container;
    }
  
    createRealNode(vnode) {
      if (typeof vnode.children === 'string') {
        return document.createTextNode(vnode.children);
      }
  
      const dom = document.createElement(vnode.tag);
      this.applyProps(dom, vnode.props);
  
      vnode.children.forEach(child => {
        const childDom = this.createRealNode(child);
        dom.appendChild(childDom);
        child.dom = childDom;
      });
  
      return dom;
    }
  
    applyProps(dom, props) {
      for (const [key, value] of Object.entries(props || {})) {
        if (key.startsWith('on')) {
          const eventType = key.slice(2).toLowerCase();
          dom.addEventListener(eventType, value);
        } else {
          dom.setAttribute(key, value);
        }
      }
    }
  
    update(nextVNode) {
      if (!this.rootVNode) throw new Error('No root VNode mounted!');
      this.batchQueue.push(nextVNode);
  
      if (!this.batchScheduled) {
        this.batchScheduled = true;
        Promise.resolve().then(() => {
          const nextVNode = this.batchQueue.pop();
          this.diff(this.rootVNode, nextVNode);
          this.batchQueue = [];
          this.batchScheduled = false;
          this.rootVNode = nextVNode;
        });
      }
    }
  
    diff(oldVNode, newVNode) {
      if (!oldVNode || !newVNode) return;
  
      if (oldVNode.tag !== newVNode.tag) {
        const newDom = this.createRealNode(newVNode);
        oldVNode.dom.replaceWith(newDom);
        newVNode.dom = newDom;
        return;
      }
  
      newVNode.dom = oldVNode.dom;
  
      this.updateProps(oldVNode.dom, oldVNode.props, newVNode.props);
  
      const oldChildren = oldVNode.children;
      const newChildren = newVNode.children;
  
      const maxLength = Math.max(oldChildren.length, newChildren.length);
      for (let i = 0; i < maxLength; i++) {
        if (!oldChildren[i]) {
          const newChildDom = this.createRealNode(newChildren[i]);
          newVNode.dom.appendChild(newChildDom);
          newChildren[i].dom = newChildDom;
        } else if (!newChildren[i]) {
          oldVNode.dom.removeChild(oldChildren[i].dom);
        } else {
          this.diff(oldChildren[i], newChildren[i]);
        }
      }
    }
  
    updateProps(dom, oldProps = {}, newProps = {}) {
      for (const [key, value] of Object.entries(newProps)) {
        if (value !== oldProps[key]) {
          if (key.startsWith('on')) {
            const eventType = key.slice(2).toLowerCase();
            dom.addEventListener(eventType, value);
          } else {
            dom.setAttribute(key, value);
          }
        }
      }
  
      for (const key of Object.keys(oldProps)) {
        if (!(key in newProps)) {
          if (key.startsWith('on')) {
            const eventType = key.slice(2).toLowerCase();
            dom.removeEventListener(eventType, oldProps[key]);
          } else {
            dom.removeAttribute(key);
          }
        }
      }
    }
  
    unmount() {
      if (this.rootVNode && this.rootDOM) {
        this.rootDOM.innerHTML = '';
        this.rootVNode = null;
        this.rootDOM = null;
      }
    }
  }
  
  // Example usage:
  const vdom = new VirtualDOM();
  const App = vdom.createElement(
    'div',
    { id: 'app' },
    vdom.createElement('h1', null, 'Hello Virtual DOM!'),
    vdom.createElement('p', null, 'This is a simple implementation.')
  );
  
  document.addEventListener('DOMContentLoaded', () => {
    const container = document.getElementById('root');
    vdom.mount(App, container);
  
    setTimeout(() => {
      const UpdatedApp = vdom.createElement(
        'div',
        { id: 'app' },
        vdom.createElement('h1', null, 'Updated Virtual DOM!'),
        vdom.createElement('p', null, 'Now with a diff algorithm.')
      );
      vdom.update(UpdatedApp);
    }, 2000);
  });
  