const gulp = require('gulp');
const marked = require('marked');
const imagemin = require('imagemin');
const jsdom = require('jsdom');
const fs = require('fs');

// Markdown Rendering
const markdown = {
  render: (text) => {
    return marked(text);
  }
};

// Template Support
const templates = {};

const template1 = `
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Template 1</title>
  <link rel="stylesheet" href="/assets/asset2.css" />
</head>
<body>
  ${templateContent}
</body>
</html>
`;

const template2 = `
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Template 2</title>
  <link rel="stylesheet" href="/assets/asset2.css" />
</head>
<body>
  ${templateContent}
</body>
</html>
`;

const renderTemplate = (templateName) => {
  const templateContent = require(`./templates/${templateName}.html`);
  return jsdom.jsdom(templateContent, { url: 'https://example.com' });
};

// Asset Pipeline
const compressAssets = () => {
  return gulp.src('assets/**/*')
    .pipe(imagemin())
    .pipe(gulp.dest('assets/'));
};

// SEO Optimization
const optimizeForSearch = () => {
  // Hier kommt die Optimierung für den Suchmaschiner

  return 'Meine Seite ist optimiert für den Suchmaschiner';
};

// Build Pipeline
const build = () => {
  const source = gulp.src('src/**/*');
  const destination = gulp.dest('build/');

  source.pipe(marked.render())
    .pipe(renderTemplate('template1'))
    .pipe(optimizeForSearch())
    .pipe(destination);
};

exports.build = build;