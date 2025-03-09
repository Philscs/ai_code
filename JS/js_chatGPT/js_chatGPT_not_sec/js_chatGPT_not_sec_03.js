const fs = require('fs');
const path = require('path');

/**
 * Generate routes recursively based on a directory structure.
 * @param {string} dirPath - The root directory path for API routes.
 * @param {object} app - Express/Fastify app instance.
 * @param {string} baseRoute - Base route path (default is '/api').
 */
function generateRoutes(dirPath, app, baseRoute = '/api') {
  const entries = fs.readdirSync(dirPath, { withFileTypes: true });

  entries.forEach((entry) => {
    const entryPath = path.join(dirPath, entry.name);

    if (entry.isDirectory()) {
      // Handle dynamic route segments (e.g., [id])
      const routeSegment = entry.name.startsWith('[') && entry.name.endsWith(']')
        ? `:${entry.name.slice(1, -1)}`
        : entry.name;

      const newBaseRoute = path.join(baseRoute, routeSegment).replace(/\\/g, '/');

      // Recurse into the directory
      generateRoutes(entryPath, app, newBaseRoute);
    } else if (entry.isFile() && entry.name.endsWith('.js')) {
      const routeHandler = require(entryPath);

      if (typeof routeHandler === 'function') {
        // Register the route handler for the current route
        app.all(baseRoute, routeHandler);
        console.log(`Registered route: ${baseRoute}`);
      } else {
        console.warn(`Skipping ${entryPath}: Export is not a function`);
      }
    }
  });
}

// Example usage with Express:
const express = require('express');
const app = express();

// Path to your API directory
const apiDirectory = path.join(__dirname, 'api');

// Generate routes
generateRoutes(apiDirectory, app);

// Start the server
const PORT = 3000;
app.listen(PORT, () => {
  console.log(`Server is running on http://localhost:${PORT}`);
});

// Example route file: /api/users/[id]/posts/[postId].js
// module.exports = (req, res) => {
//   const { id, postId } = req.params;
//   res.json({ userId: id, postId });
// };