// Virtual DOM Node Klasse
class VNode {
    constructor(type, props, children) {
      this.type = type;
      this.props = props || {};
      this.children = children || [];
      this.key = props ? props.key : null;
      this.ref = props ? props.ref : null;
      this.element = null;
      this.lifecycle = {
        mounted: false,
        updated: false
      };
    }
  }
  
  // Lifecycle Hooks Manager
  class LifecycleManager {
    constructor() {
      this.hooks = new Map();
    }
  
    addHook(node, type, callback) {
      if (!this.hooks.has(node)) {
        this.hooks.set(node, new Map());
      }
      const nodeHooks = this.hooks.get(node);
      if (!nodeHooks.has(type)) {
        nodeHooks.set(type, []);
      }
      nodeHooks.get(type).push(callback);
    }
  
    triggerHook(node, type) {
      if (this.hooks.has(node)) {
        const nodeHooks = this.hooks.get(node);
        if (nodeHooks.has(type)) {
          nodeHooks.get(type).forEach(callback => callback());
        }
      }
    }
  }
  
  // Event Delegation Manager
  class EventManager {
    constructor(rootElement) {
      this.rootElement = rootElement;
      this.eventMap = new Map();
      this.setupDelegation();
    }
  
    setupDelegation() {
      this.rootElement.addEventListener('click', (e) => this.handleEvent(e, 'click'));
      this.rootElement.addEventListener('input', (e) => this.handleEvent(e, 'input'));
      // Weitere Event-Typen können hier hinzugefügt werden
    }
  
    handleEvent(event, type) {
      let target = event.target;
      while (target && target !== this.rootElement) {
        const handlers = this.eventMap.get(target);
        if (handlers && handlers[type]) {
          handlers[type](event);
          break;
        }
        target = target.parentNode;
      }
    }
  
    addHandler(element, type, handler) {
      if (!this.eventMap.has(element)) {
        this.eventMap.set(element, {});
      }
      this.eventMap.get(element)[type] = handler;
    }
  }
  
  // Batch Update Manager
  class BatchManager {
    constructor() {
      this.updates = new Set();
      this.isPending = false;
    }
  
    queueUpdate(node) {
      this.updates.add(node);
      if (!this.isPending) {
        this.isPending = true;
        requestAnimationFrame(() => this.processUpdates());
      }
    }
  
    processUpdates() {
      this.updates.forEach(node => {
        renderer.updateNode(node);
      });
      this.updates.clear();
      this.isPending = false;
    }
  }
  
  // Virtual DOM Renderer
  class VDOMRenderer {
    constructor(container) {
      this.container = container;
      this.lifecycleManager = new LifecycleManager();
      this.eventManager = new EventManager(container);
      this.batchManager = new BatchManager();
    }
  
    createElement(vnode) {
      if (typeof vnode === 'string' || typeof vnode === 'number') {
        return document.createTextNode(vnode);
      }
  
      const element = document.createElement(vnode.type);
      
      // Props verarbeiten
      Object.entries(vnode.props || {}).forEach(([key, value]) => {
        if (key.startsWith('on')) {
          const eventType = key.toLowerCase().slice(2);
          this.eventManager.addHandler(element, eventType, value);
        } else if (key === 'className') {
          element.className = value;
        } else if (key !== 'key' && key !== 'ref') {
          element.setAttribute(key, value);
        }
      });
  
      // Kinder rekursiv erstellen
      vnode.children.forEach(child => {
        element.appendChild(this.createElement(child));
      });
  
      vnode.element = element;
      this.lifecycleManager.triggerHook(vnode, 'mounted');
      vnode.lifecycle.mounted = true;
  
      return element;
    }
  
    diff(oldVNode, newVNode) {
      if (!oldVNode || !newVNode) {
        return true;
      }
  
      if (typeof oldVNode !== typeof newVNode) {
        return true;
      }
  
      if (typeof oldVNode === 'string' || typeof oldVNode === 'number') {
        return oldVNode !== newVNode;
      }
  
      if (oldVNode.type !== newVNode.type) {
        return true;
      }
  
      const propsChanged = this.diffProps(oldVNode.props, newVNode.props);
      const childrenChanged = this.diffChildren(oldVNode.children, newVNode.children);
  
      return propsChanged || childrenChanged;
    }
  
    diffProps(oldProps, newProps) {
      oldProps = oldProps || {};
      newProps = newProps || {};
      
      return (
        Object.keys(oldProps).length !== Object.keys(newProps).length ||
        Object.keys(oldProps).some(key => oldProps[key] !== newProps[key])
      );
    }
  
    diffChildren(oldChildren, newChildren) {
      if (oldChildren.length !== newChildren.length) {
        return true;
      }
  
      return oldChildren.some((child, i) => this.diff(child, newChildren[i]));
    }
  
    updateNode(vnode) {
      if (!vnode.element) {
        return;
      }
  
      // Props aktualisieren
      Object.entries(vnode.props || {}).forEach(([key, value]) => {
        if (key.startsWith('on')) {
          const eventType = key.toLowerCase().slice(2);
          this.eventManager.addHandler(vnode.element, eventType, value);
        } else if (key === 'className') {
          vnode.element.className = value;
        } else if (key !== 'key' && key !== 'ref') {
          vnode.element.setAttribute(key, value);
        }
      });
  
      this.lifecycleManager.triggerHook(vnode, 'updated');
      vnode.lifecycle.updated = true;
    }
  
    render(vnode) {
      const element = this.createElement(vnode);
      this.container.appendChild(element);
      return element;
    }
  
    // Öffentliche API für Lifecycle Hooks
    onMount(vnode, callback) {
      this.lifecycleManager.addHook(vnode, 'mounted', callback);
    }
  
    onUpdate(vnode, callback) {
      this.lifecycleManager.addHook(vnode, 'updated', callback);
    }
  
    // Öffentliche API für Updates
    queueUpdate(vnode) {
      this.batchManager.queueUpdate(vnode);
    }
  }
  
  // Beispiel-Nutzung:
  const container = document.getElementById('app');
  const renderer = new VDOMRenderer(container);
  
  // Beispiel-Component erstellen
  const myComponent = new VNode('div', { className: 'container' }, [
    new VNode('h1', { className: 'title' }, ['Hallo Welt']),
    new VNode('button', {
      className: 'btn',
      onClick: () => console.log('Clicked!')
    }, ['Klick mich'])
  ]);
  
  // Lifecycle Hook hinzufügen
  renderer.onMount(myComponent, () => {
    console.log('Component wurde gemounted!');
  });
  
  // Component rendern
  renderer.render(myComponent);
  
  // Update queuen
  renderer.queueUpdate(myComponent);