class RouteGenerator {
    constructor(rootPath = '', middlewares = []) {
      this.rootPath = rootPath;
      this.middlewares = middlewares;
      this.routes = [];
    }
  
    addRoute(path, handler) {
      this.routes.push({
        path,
        handler,
      });
    }
  
    async generateRoutes() {
      const routes = [];
  
      // Registriere alle Unterverzeichnisse
      for (const dir of Object.keys(require('path').dirname(this.rootPath))) {
        if (dir !== '') {
          await this.generateSubRoute(dir);
        }
      }
  
      return routes;
    }
  
    async generateSubRoute(subDir) {
      const subDirPath = `${this.rootPath}/${subDir}`;
      const subRoutes = [];
  
      // Registriere alle Unterverzeichnisse
      for (const dir of Object.keys(require('path').dirname(subDirPath))) {
        if (dir !== '') {
          await this.generateSubRoute(dir);
        }
  
        // Registriere Routen für diese Verzeichnisstruktur
        const path = `${subDirPath}/${dir}`;
        subRoutes.push({
          path,
        });
  
        // Registriere Routen für dieses spezifische Verzeichnis
        if (this.routes.find(route => route.path === path)) {
          continue;
        }
  
        this.addRoute(path, (req, res) => {
          return this.generateSubRouteHandler(req.params);
        });
      }
  
      subRoutes.push({
        path: `${subDirPath}/${dir}`,
        handler: async (req, res) => {
          // Hier kannst du eine Routen-Handling implementieren
          res.send(`Willkommen in ${path}`);
        },
      });
  
      return subRoutes;
    }
  
    async generateSubRouteHandler(params) {
      const subDir = params.subDir || '';
      let path = `${this.rootPath}/${subDir}`;
  
      if (params.postId) {
        path += `/${params.postId}`;
      }
  
      // Registriere Routen für diese Verzeichnisstruktur
      for (const route of this.routes) {
        const subRoute = Object.assign({}, route);
        subRoute.path = `${path}/${subRoute.path}`;
  
        if (route.handler && typeof route.handler === 'function') {
          return async (req, res) => await route.handler(req, res);
        }
  
        delete subRoute.handler;
      }
  
      // Hier kannst du eine Route-Handling implementieren
      return res.send(`Willkommen in ${path}`);
    }
  }
  
  module.exports = RouteGenerator;