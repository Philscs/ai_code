// Benötigte Abhängigkeiten installieren mit: 
// npm install marked ejs fs-extra

const fs = require('fs-extra');
const path = require('path');
const marked = require('marked');
const ejs = require('ejs');

const SOURCE_DIR = path.join(__dirname, 'src'); // Quellverzeichnis
const BUILD_DIR = path.join(__dirname, 'dist'); // Zielverzeichnis
const TEMPLATE_FILE = path.join(SOURCE_DIR, 'template.ejs');
const ASSETS_DIR = path.join(SOURCE_DIR, 'assets');
const OUTPUT_ASSETS_DIR = path.join(BUILD_DIR, 'assets');

async function processMarkdown(filePath) {
    const content = await fs.readFile(filePath, 'utf-8');
    return marked(content); // Markdown in HTML umwandeln
}

async function renderTemplate(templatePath, data) {
    const template = await fs.readFile(templatePath, 'utf-8');
    return ejs.render(template, data);
}

async function copyAssets() {
    if (await fs.pathExists(ASSETS_DIR)) {
        await fs.copy(ASSETS_DIR, OUTPUT_ASSETS_DIR);
    }
}

async function buildSite() {
    try {
        // Zielverzeichnis leeren
        await fs.emptyDir(BUILD_DIR);

        // Markdown-Dateien verarbeiten
        const files = await fs.readdir(SOURCE_DIR);

        for (const file of files) {
            const ext = path.extname(file);
            if (ext === '.md') {
                const filePath = path.join(SOURCE_DIR, file);
                const htmlContent = await processMarkdown(filePath);
                const outputFileName = file.replace('.md', '.html');
                const outputPath = path.join(BUILD_DIR, outputFileName);

                const finalHtml = await renderTemplate(TEMPLATE_FILE, {
                    content: htmlContent,
                    title: path.basename(file, '.md'),
                });

                await fs.writeFile(outputPath, finalHtml, 'utf-8');
            }
        }

        // Assets kopieren
        await copyAssets();

        console.log('Website erfolgreich generiert!');
    } catch (error) {
        console.error('Fehler beim Generieren der Website:', error);
    }
}

buildSite();

// Struktur für src-Ordner:
// src/
//   |-- assets/ (z.B. CSS, Bilder, JS-Dateien)
//   |-- template.ejs (HTML-Template, z.B. mit <%- content %> für den Inhalt)
//   |-- beispiel.md (Markdown-Dateien mit dem Inhalt)

// SEO Optimization kann in das Template (template.ejs) eingebaut werden:
// <title><%= title %></title>
// <meta name="description" content="Beschreibung der Seite">
// <meta name="keywords" content="Keyword1, Keyword2">
