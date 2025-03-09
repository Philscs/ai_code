function diffArray(oldArray, newArray) {
    const result = [];
    let i = 0;
    let j = 0;
  
    while (i < oldArray.length && j < newArray.length) {
      if (oldArray[i] === newArray[j]) {
        i++;
        j++;
      } else if (!newArray.includes(oldArray[i])) {
        result.push(newArray[j]);
        j++;
      } else {
        result.push(oldArray[i]);
        i++;
      }
    }
  
    while (i < oldArray.length) {
      result.push(oldArray[i]);
      i++;
    }
  
    while (j < newArray.length) {
      result.push(newArray[j]);
      j++;
    }
  
    return result;
  }
  class UpdateQueue {
    constructor() {
      this.queue = [];
    }
  
    push(value) {
      this.queue.push(value);
    }
  
    process() {
      if (this.queue.length > 0) {
        const oldArray = this.queue.shift();
        const newArray = []; // berechnen Sie hier die neue Array-Elemente
  
        const result = diffArray(oldArray, newArray);
  
        // aktualisieren Sie hier die DOM-Elemente
        // Beispiel:
        for (const element of result) {
          element.parentNode.replaceChild(element, this.queue.shift());
        }
  
        if (this.queue.length > 0) {
          this.process();
        }
      }
    }
  }
  class Component {
    constructor() {
      this.state = {}; // initialisiere hier den Zustand
      this.updating = false; // initialisiere hier das updating-Flag
  
      this.updateQueue = new UpdateQueue();
  
      this.componentDidMount();
    }
  
    componentDidMount() {
      this.updateQueue.push(this.state); // add current state to queue
    }
  
    componentWillUpdate(nextProps, nextState) {
      if (nextState !== this.state) {
        this.setState(nextState);
      }
    }
  
    componentDidUpdate(prevProps, prevState) {
      if (prevProps !== this.props || prevState !== this.state) {
        this.updateQueue.push(this.state); // add current state to queue
      }
    }
  
    componentWillUnmount() {
      this.updating = false; // set updating flag to false
    }
  
    render() {
      return <div>{this.renderChildren()}</div>;
    }
  
    renderChildren() {
      const { children } = this.props;
      return children.map((child) => child.render());
    }
  }
  class EventDelegator {
    constructor(element) {
      this.element = element;
      this.listenedEvents = {};
    }
  
    listen(event, handler) {
      if (!this.listenedEvents[event]) {
        this.listenedEvents[event] = [];
      }
      this.listenedEvents[event].push(handler);
    }
  
    off(event, handler) {
      const index = this.listenedEvents[event].indexOf(handler);
      if (index !== -1) {
        this.listenedEvents[event].splice(index, 1);
      }
    }
  
    handleEvent(event) {
      for (const handler of this.listenedEvents[event]) {
        handler(event);
      }
    }
  }