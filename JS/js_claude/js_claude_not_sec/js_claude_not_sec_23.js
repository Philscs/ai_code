// Router Class Implementation
class Router {
    constructor(options = {}) {
      this.routes = new Map();
      this.currentRoute = null;
      this.guards = new Map();
      this.navigationCallbacks = [];
      this.baseUrl = options.baseUrl || '';
      
      // Initialize router
      window.addEventListener('popstate', this.handlePopState.bind(this));
      this.initializeRouter();
    }
  
    // Route registration with nested routes support
    register(path, component, options = {}) {
      const route = {
        path,
        component,
        children: new Map(),
        guards: options.guards || [],
        lazy: options.lazy || false
      };
      
      if (path.includes('/')) {
        this.registerNestedRoute(path, route);
      } else {
        this.routes.set(path, route);
      }
      
      return this;
    }
  
    // Register nested routes
    registerNestedRoute(path, route) {
      const segments = path.split('/').filter(Boolean);
      let currentLevel = this.routes;
      
      for (let i = 0; i < segments.length - 1; i++) {
        const segment = segments[i];
        if (!currentLevel.has(segment)) {
          currentLevel.set(segment, {
            path: segment,
            children: new Map()
          });
        }
        currentLevel = currentLevel.get(segment).children;
      }
      
      currentLevel.set(segments[segments.length - 1], route);
    }
  
    // Route guard registration
    registerGuard(name, guardFn) {
      this.guards.set(name, guardFn);
      return this;
    }
  
    // Navigation event subscription
    onNavigation(callback) {
      this.navigationCallbacks.push(callback);
      return this;
    }
  
    // Parse URL parameters
    parseParams(path) {
      const params = {};
      const segments = path.split('/');
      const routeSegments = this.findRouteSegments(segments);
      
      if (routeSegments) {
        routeSegments.forEach((routeSegment, index) => {
          if (routeSegment.startsWith(':')) {
            params[routeSegment.slice(1)] = segments[index];
          }
        });
      }
      
      return params;
    }
  
    // Find matching route segments
    findRouteSegments(segments) {
      let currentLevel = this.routes;
      const matchedSegments = [];
      
      for (const segment of segments) {
        let matched = false;
        
        for (const [routeSegment, route] of currentLevel.entries()) {
          if (routeSegment === segment || routeSegment.startsWith(':')) {
            matchedSegments.push(routeSegment);
            currentLevel = route.children;
            matched = true;
            break;
          }
        }
        
        if (!matched) return null;
      }
      
      return matchedSegments;
    }
  
    // Navigate to a new route
    async navigate(path) {
      const fullPath = this.baseUrl + path;
      const route = await this.findRoute(path);
      
      if (!route) {
        throw new Error(`Route not found: ${path}`);
      }
  
      // Check route guards
      for (const guardName of route.guards) {
        const guard = this.guards.get(guardName);
        if (guard && !(await guard(route))) {
          return false;
        }
      }
  
      // Handle lazy loading
      if (route.lazy && typeof route.component === 'function') {
        route.component = await route.component();
      }
  
      // Update browser history
      window.history.pushState({}, '', fullPath);
      
      // Update current route
      this.currentRoute = {
        ...route,
        params: this.parseParams(path)
      };
  
      // Notify navigation listeners
      this.navigationCallbacks.forEach(callback => 
        callback(this.currentRoute)
      );
  
      return true;
    }
  
    // Find route by path
    async findRoute(path) {
      const segments = path.split('/').filter(Boolean);
      let currentLevel = this.routes;
      let currentRoute = null;
      
      for (const segment of segments) {
        let matched = false;
        
        for (const [routeSegment, route] of currentLevel.entries()) {
          if (routeSegment === segment || routeSegment.startsWith(':')) {
            currentRoute = route;
            currentLevel = route.children;
            matched = true;
            break;
          }
        }
        
        if (!matched) return null;
      }
      
      return currentRoute;
    }
  
    // Handle browser back/forward navigation
    handlePopState() {
      const path = window.location.pathname.slice(this.baseUrl.length);
      this.navigate(path);
    }
  
    // Initialize router with current URL
    initializeRouter() {
      const path = window.location.pathname.slice(this.baseUrl.length);
      this.navigate(path);
    }
  }
  
  // Usage Example
  const router = new Router({ baseUrl: '' });
  
  // Register route guards
  router.registerGuard('auth', async () => {
    // Check if user is authenticated
    return localStorage.getItem('token') !== null;
  });
  
  // Register routes
  router.register('/', {
    component: HomeComponent
  })
  .register('/about', {
    component: AboutComponent
  })
  .register('/dashboard', {
    component: DashboardComponent,
    guards: ['auth']
  })
  .register('/users/:id/profile', {
    component: UserProfileComponent,
    lazy: true
  });
  
  // Subscribe to navigation events
  router.onNavigation((route) => {
    console.log('Navigation occurred:', route);
  });
  
  // Example of lazy loading component
  const UserProfileComponent = async () => {
    const module = await import('./components/UserProfile.js');
    return module.default;
  };
  
  // Navigate programmatically
  router.navigate('/dashboard');