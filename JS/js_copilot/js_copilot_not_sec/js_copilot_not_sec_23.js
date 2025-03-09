// BEGIN: Router implementation
class Router {
  constructor() {
    this.routes = [];
    this.currentRoute = null;
  }

  addRoute(path, component, options = {}) {
    this.routes.push({ path, component, options });
  }

  navigateTo(path) {
    const route = this.routes.find((r) => r.path === path);
    if (route) {
      if (this.currentRoute && this.currentRoute.options.beforeLeave) {
        this.currentRoute.options.beforeLeave();
      }
      this.currentRoute = route;
      if (this.currentRoute.options.beforeEnter) {
        this.currentRoute.options.beforeEnter();
      }
      this.renderComponent(this.currentRoute.component);
    } else {
      console.error(`Route not found: ${path}`);
    }
  }

  renderComponent(component) {
    // Render the component to the DOM
    console.log(`Rendering component: ${component}`);
  }
}

// Usage
const router = new Router();

// Add routes
router.addRoute('/', 'HomeComponent');
router.addRoute('/about', 'AboutComponent');
router.addRoute('/products', 'ProductsComponent');
router.addRoute('/products/:id', 'ProductDetailComponent');

// Navigate to a route
router.navigateTo('/about');
// END: Router implementation
