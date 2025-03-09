const express = require('express');
const fs = require('fs');
const path = require('path');

function generateRouteHandlers(directory, app) {
  const files = fs.readdirSync(directory);

  files.forEach((file) => {
    const filePath = path.join(directory, file);
    const stat = fs.statSync(filePath);

    if (stat.isDirectory()) {
      const route = `/${file}`;
      const subDirectory = path.join(directory, file);
      generateRouteHandlers(subDirectory, app);

      app.use(route, express.Router());
    } else {
      const route = `/${file.replace('.js', '')}`;
      const routeHandler = require(filePath);
      app.use(route, routeHandler);
    }
  });
}

const app = express();
generateRouteHandlers('/api', app);

app.listen(3000, () => {
  console.log('Server started on port 3000');
});
