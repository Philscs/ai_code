class VirtualDOM {
    constructor() {
        this.rootInstance = null;
        this.updateQueue = [];
        this.isBatchingUpdates = false;
    }

    createElement(type, props = {}, ...children) {
        return { type, props: { ...props, children: children.flat() } };
    }

    render(vnode, container) {
        if (!this.rootInstance) {
            this.rootInstance = this.instantiate(vnode);
            container.appendChild(this.rootInstance.dom);
        } else {
            const newInstance = this.reconcile(container, this.rootInstance, vnode);
            this.rootInstance = newInstance;
        }
    }

    instantiate(vnode) {
        const { type, props } = vnode;
        
        const isDOMElement = typeof type === 'string';
        if (isDOMElement) {
            const dom = document.createElement(type);
            this.updateProps(dom, {}, props);

            const childInstances = props.children.map(child => this.instantiate(child));
            childInstances.forEach(childInstance => dom.appendChild(childInstance.dom));

            return { dom, element: vnode, childInstances };
        } else if (typeof type === 'function') {
            const instance = {};
            const component = new type(props);
            instance.dom = component.render();
            component._internalInstance = instance;

            if (component.componentDidMount) {
                setTimeout(() => component.componentDidMount(), 0);
            }

            instance.component = component;
            instance.element = vnode;
            return instance;
        }
    }

    reconcile(parentDom, instance, element) {
        if (!instance) {
            const newInstance = this.instantiate(element);
            parentDom.appendChild(newInstance.dom);
            return newInstance;
        } else if (!element) {
            if (instance.component && instance.component.componentWillUnmount) {
                instance.component.componentWillUnmount();
            }
            parentDom.removeChild(instance.dom);
            return null;
        } else if (instance.element.type !== element.type) {
            const newInstance = this.instantiate(element);
            parentDom.replaceChild(newInstance.dom, instance.dom);
            return newInstance;
        } else if (typeof element.type === 'string') {
            this.updateProps(instance.dom, instance.element.props, element.props);

            const newChildInstances = [];
            const count = Math.max(
                instance.childInstances.length,
                element.props.children.length
            );
            for (let i = 0; i < count; i++) {
                newChildInstances[i] = this.reconcile(
                    instance.dom,
                    instance.childInstances[i],
                    element.props.children[i]
                );
            }

            instance.childInstances = newChildInstances.filter(child => child !== null);
            instance.element = element;
            return instance;
        }
    }

    updateProps(dom, prevProps, nextProps) {
        const isEvent = key => key.startsWith('on');
        const isProperty = key => key !== 'children' && !isEvent(key);

        Object.keys(prevProps).forEach(name => {
            if (isEvent(name)) {
                dom.removeEventListener(name.toLowerCase().substring(2), prevProps[name]);
            } else if (isProperty(name)) {
                dom[name] = '';
            }
        });

        Object.keys(nextProps).forEach(name => {
            if (isEvent(name)) {
                dom.addEventListener(name.toLowerCase().substring(2), nextProps[name]);
            } else if (isProperty(name)) {
                dom[name] = nextProps[name];
            }
        });
    }

    batchUpdate(updateFn) {
        this.updateQueue.push(updateFn);
        if (!this.isBatchingUpdates) {
            this.isBatchingUpdates = true;
            setTimeout(() => {
                this.updateQueue.forEach(fn => fn());
                this.updateQueue = [];
                this.isBatchingUpdates = false;
            }, 0);
        }
    }
}

class Component {
    constructor(props) {
        this.props = props;
        this.state = {};
    }

    setState(partialState) {
        VirtualDOM.batchUpdate(() => {
            this.state = { ...this.state, ...partialState };
            const parentDom = this._internalInstance.dom.parentNode;
            const newElement = this.render();
            VirtualDOM.reconcile(parentDom, this._internalInstance, newElement);
        });
    }

    render() {
        throw new Error('Components must implement render()');
    }
}

const VirtualDOM = new VirtualDOM();
