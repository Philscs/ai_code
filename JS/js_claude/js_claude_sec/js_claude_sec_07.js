// Virtual DOM Node Klasse
class VNode {
    constructor(type, props = {}, children = []) {
      this.type = type;
      this.props = props;
      this.children = children;
      this.key = props.key || null;
      this.ref = props.ref || null;
      this._instance = null;
      this._dom = null;
    }
  }
  
  // Component Basisklasse
  class Component {
    constructor(props = {}) {
      this.props = props;
      this.state = {};
      this._pendingState = null;
      this._vnode = null;
      this._dom = null;
      this._dirty = false;
      this._mounted = false;
    }
  
    setState(partialState) {
      this._pendingState = {
        ...this._pendingState || this.state,
        ...(typeof partialState === 'function' ? partialState(this._pendingState || this.state) : partialState)
      };
      
      if (!this._dirty) {
        this._dirty = true;
        updateQueue.push(this);
      }
    }
  
    forceUpdate() {
      if (this._pendingState) {
        this.state = this._pendingState;
        this._pendingState = null;
      }
      
      const oldVNode = this._vnode;
      const newVNode = this.render();
      
      this._vnode = patch(this._dom.parentNode, oldVNode, newVNode);
    }
  
    // Lifecycle Methoden
    componentWillMount() {}
    componentDidMount() {}
    componentWillUpdate() {}
    componentDidUpdate() {}
    componentWillUnmount() {}
  }
  
  // Update Queue für Batch Processing
  const updateQueue = [];
  let updateScheduled = false;
  
  function scheduleUpdate() {
    if (!updateScheduled) {
      updateScheduled = true;
      Promise.resolve().then(processUpdateQueue);
    }
  }
  
  function processUpdateQueue() {
    updateScheduled = false;
    const components = new Set(updateQueue);
    updateQueue.length = 0;
    
    for (const component of components) {
      if (component._dirty) {
        component._dirty = false;
        component.forceUpdate();
      }
    }
  }
  
  // Diff Algorithmus
  function patch(parent, oldVNode, newVNode) {
    // Gleicher Typ -> Update
    if (oldVNode && newVNode && oldVNode.type === newVNode.type) {
      if (typeof oldVNode.type === 'function') {
        return patchComponent(parent, oldVNode, newVNode);
      } else {
        return patchElement(parent, oldVNode, newVNode);
      }
    } 
    // Unterschiedlicher Typ -> Replace
    else {
      const newDom = create(newVNode);
      if (oldVNode && oldVNode._dom) {
        parent.replaceChild(newDom, oldVNode._dom);
      } else {
        parent.appendChild(newDom);
      }
      return newVNode;
    }
  }
  
  function patchElement(parent, oldVNode, newVNode) {
    const dom = newVNode._dom = oldVNode._dom;
    const oldProps = oldVNode.props;
    const newProps = newVNode.props;
  
    // Update Props
    for (const name in newProps) {
      if (oldProps[name] !== newProps[name]) {
        setProp(dom, name, newProps[name]);
      }
    }
    
    // Remove old props
    for (const name in oldProps) {
      if (!(name in newProps)) {
        removeProp(dom, name);
      }
    }
  
    // Patch Children mit Key-basiertem Algorithmus
    patchChildren(dom, oldVNode.children, newVNode.children);
    
    return newVNode;
  }
  
  function patchChildren(parent, oldChildren, newChildren) {
    const oldKeyedChildren = new Map();
    const newKeyedChildren = new Map();
    
    // Build key maps
    oldChildren.forEach((child, i) => {
      if (child.key) oldKeyedChildren.set(child.key, {vnode: child, index: i});
    });
    
    newChildren.forEach((child, i) => {
      if (child.key) newKeyedChildren.set(child.key, {vnode: child, index: i});
    });
    
    // Remove old keyed children that aren't in new set
    oldKeyedChildren.forEach((child, key) => {
      if (!newKeyedChildren.has(key)) {
        parent.removeChild(child.vnode._dom);
      }
    });
    
    // Place new children
    let lastIndex = 0;
    newChildren.forEach((newChild, i) => {
      const key = newChild.key;
      const oldChild = key ? oldKeyedChildren.get(key) : null;
      
      if (oldChild) {
        patch(parent, oldChild.vnode, newChild);
        if (oldChild.index < lastIndex) {
          // Node needs to move forward
          const refNode = newChildren[i - 1]?._dom.nextSibling;
          parent.insertBefore(newChild._dom, refNode);
        }
        lastIndex = Math.max(lastIndex, oldChild.index);
      } else {
        // Insert new node
        const dom = create(newChild);
        const refNode = i === 0 ? parent.firstChild : newChildren[i - 1]._dom.nextSibling;
        parent.insertBefore(dom, refNode);
      }
    });
  }
  
  // Event Delegation System
  const eventProxy = event => {
    let target = event.target;
    const eventType = `on${event.type}`;
    
    while (target) {
      const handler = target._vnode?.props[eventType];
      if (handler) {
        handler.call(null, event);
        if (event.cancelBubble) break;
      }
      target = target.parentNode;
    }
  };
  
  // Globale Event-Listener für Delegation
  const delegatedEvents = new Set(['click', 'input', 'change', 'submit']);
  delegatedEvents.forEach(type => {
    document.addEventListener(type, eventProxy);
  });
  
  // Memory Management & Cleanup
  function destroyVNode(vnode) {
    if (!vnode) return;
    
    // Cleanup component instance
    if (vnode._instance) {
      vnode._instance.componentWillUnmount();
      vnode._instance._dom = null;
      vnode._instance._vnode = null;
      vnode._instance = null;
    }
    
    // Remove event listeners
    if (vnode._dom) {
      const props = vnode.props;
      for (const name in props) {
        if (name.startsWith('on') && !delegatedEvents.has(name.toLowerCase().slice(2))) {
          const eventType = name.toLowerCase().slice(2);
          vnode._dom.removeEventListener(eventType, props[name]);
        }
      }
    }
    
    // Recursive cleanup of children
    if (vnode.children) {
      vnode.children.forEach(destroyVNode);
    }
    
    // Clear references
    vnode._dom = null;
  }
  
  // Hilfsfunktionen
  function createElement(type, props, ...children) {
    return new VNode(type, props, children.flat());
  }
  
  function setProp(dom, name, value) {
    if (name === 'key' || name === 'ref') return;
    
    if (name.startsWith('on') && typeof value === 'function') {
      const eventType = name.toLowerCase().slice(2);
      if (!delegatedEvents.has(eventType)) {
        dom.addEventListener(eventType, value);
      }
    } else if (name === 'style' && typeof value === 'object') {
      Object.assign(dom.style, value);
    } else if (typeof value === 'boolean') {
      if (value) dom.setAttribute(name, '');
      else dom.removeAttribute(name);
    } else {
      dom.setAttribute(name, value);
    }
  }
  
  function removeProp(dom, name) {
    if (name.startsWith('on') && typeof value === 'function') {
      const eventType = name.toLowerCase().slice(2);
      if (!delegatedEvents.has(eventType)) {
        dom.removeEventListener(eventType, value);
      }
    } else if (typeof value === 'boolean') {
      dom.removeAttribute(name);
    } else {
      dom.removeAttribute(name);
    }
  }
  
  // Export API
  export {
    createElement,
    Component,
    patch,
    scheduleUpdate
  };