
const fs = require('fs');
const path = require('path');
const marked = require('marked');
const handlebars = require('handlebars');

// Define the source and output directories
const sourceDir = 'src';
const outputDir = 'dist';

// Read the source files
const files = fs.readdirSync(sourceDir);

// Process each file
files.forEach(file => {
  const filePath = path.join(sourceDir, file);
  const fileContent = fs.readFileSync(filePath, 'utf8');

  // Process Markdown
  const htmlContent = marked(fileContent);

  // Apply template
  const template = fs.readFileSync('template.html', 'utf8');
  const compiledTemplate = handlebars.compile(template);
  const outputContent = compiledTemplate({ content: htmlContent });

  // Write the output file
  const outputFilePath = path.join(outputDir, file.replace('.md', '.html'));
  fs.writeFileSync(outputFilePath, outputContent, 'utf8');
});

// Copy assets
const assetsDir = 'assets';
const outputAssetsDir = path.join(outputDir, assetsDir);
fs.mkdirSync(outputAssetsDir, { recursive: true });
fs.readdirSync(assetsDir).forEach(file => {
  const assetFilePath = path.join(assetsDir, file);
  const outputAssetFilePath = path.join(outputAssetsDir, file);
  fs.copyFileSync(assetFilePath, outputAssetFilePath);
});

console.log('Site generated successfully!');
