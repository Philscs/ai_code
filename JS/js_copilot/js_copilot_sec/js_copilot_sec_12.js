// BEGIN: Two-Way Binding
class TwoWayBinding {
  constructor() {
    this.bindings = new Map();
  }

  bind(element, property, callback) {
    const value = element[property];
    this.bindings.set(element, { property, callback });
    element[property] = value;

    element.addEventListener('input', () => {
      const binding = this.bindings.get(element);
      const newValue = element[property];
      if (newValue !== binding.value) {
        binding.value = newValue;
        callback(newValue);
      }
    });
  }
}
// END: Two-Way Binding
