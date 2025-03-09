// src/index.js
import fs from 'fs/promises';
import path from 'path';
import marked from 'marked';
import frontMatter from 'front-matter';
import handlebars from 'handlebars';
import { minify } from 'html-minifier';
import sharp from 'sharp';
import sitemap from 'sitemap';
import { glob } from 'glob';

class StaticSiteGenerator {
    constructor(config = {}) {
        this.config = {
            sourcePath: 'content',
            outputPath: 'public',
            templatesPath: 'templates',
            assetsPath: 'assets',
            baseUrl: 'https://example.com',
            ...config
        };
    }

    async initialize() {
        // Create necessary directories
        await fs.mkdir(this.config.outputPath, { recursive: true });
        await fs.mkdir(path.join(this.config.outputPath, 'assets'), { recursive: true });
    }

    async processMarkdown(content) {
        // Parse frontmatter and markdown
        const { attributes, body } = frontMatter(content);
        const htmlContent = marked(body);
        return { metadata: attributes, content: htmlContent };
    }

    async loadTemplate(templateName) {
        const templatePath = path.join(this.config.templatesPath, `${templateName}.hbs`);
        const templateContent = await fs.readFile(templatePath, 'utf-8');
        return handlebars.compile(templateContent);
    }

    async optimizeImage(inputPath, outputPath) {
        await sharp(inputPath)
            .resize(1200, null, { withoutEnlargement: true })
            .jpeg({ quality: 80, progressive: true })
            .webp({ quality: 80 })
            .toFile(outputPath);
    }

    async processAssets() {
        const assetFiles = await glob('**/*', { 
            cwd: this.config.assetsPath,
            nodir: true 
        });

        for (const file of assetFiles) {
            const inputPath = path.join(this.config.assetsPath, file);
            const outputPath = path.join(this.config.outputPath, 'assets', file);
            const ext = path.extname(file).toLowerCase();

            // Create output directory if it doesn't exist
            await fs.mkdir(path.dirname(outputPath), { recursive: true });

            // Process images
            if (['.jpg', '.jpeg', '.png'].includes(ext)) {
                await this.optimizeImage(inputPath, outputPath);
            } else {
                // Copy other assets as-is
                await fs.copyFile(inputPath, outputPath);
            }
        }
    }

    generateSEOMetadata(metadata, content) {
        return {
            title: metadata.title,
            description: metadata.description || content.substring(0, 160),
            keywords: metadata.keywords || [],
            canonical: new URL(metadata.slug, this.config.baseUrl).toString(),
            ogTags: {
                title: metadata.title,
                description: metadata.description,
                image: metadata.featuredImage,
                type: 'article'
            }
        };
    }

    async generateSitemap(pages) {
        const smStream = new sitemap.SitemapStream({ 
            hostname: this.config.baseUrl 
        });

        pages.forEach(page => {
            smStream.write({
                url: page.slug,
                changefreq: page.changefreq || 'weekly',
                priority: page.priority || 0.5
            });
        });

        smStream.end();

        const sitemapPath = path.join(this.config.outputPath, 'sitemap.xml');
        await fs.writeFile(sitemapPath, await sitemap.streamToPromise(smStream));
    }

    async build() {
        try {
            await this.initialize();

            // Get all markdown files
            const contentFiles = await glob('**/*.md', { 
                cwd: this.config.sourcePath 
            });

            const pages = [];

            // Process each markdown file
            for (const file of contentFiles) {
                const content = await fs.readFile(
                    path.join(this.config.sourcePath, file), 
                    'utf-8'
                );

                // Process markdown and frontmatter
                const { metadata, content: htmlContent } = await this.processMarkdown(content);

                // Load and apply template
                const template = await this.loadTemplate(metadata.template || 'default');
                
                // Generate SEO metadata
                const seoData = this.generateSEOMetadata(metadata, htmlContent);

                // Combine everything into the final HTML
                const finalHtml = template({
                    content: htmlContent,
                    metadata: metadata,
                    seo: seoData
                });

                // Minify HTML
                const minifiedHtml = minify(finalHtml, {
                    collapseWhitespace: true,
                    removeComments: true,
                    minifyCSS: true,
                    minifyJS: true
                });

                // Calculate output path
                const outputFilePath = path.join(
                    this.config.outputPath,
                    file.replace('.md', '.html')
                );

                // Create directory if it doesn't exist
                await fs.mkdir(path.dirname(outputFilePath), { recursive: true });

                // Write the file
                await fs.writeFile(outputFilePath, minifiedHtml);

                // Store page info for sitemap
                pages.push({
                    slug: file.replace('.md', ''),
                    ...metadata
                });
            }

            // Process assets
            await this.processAssets();

            // Generate sitemap
            await this.generateSitemap(pages);

            console.log('Build completed successfully!');
        } catch (error) {
            console.error('Build failed:', error);
            throw error;
        }
    }
}

// Example usage:
const generator = new StaticSiteGenerator({
    baseUrl: 'https://mysite.com',
    sourcePath: 'content',
    outputPath: 'public',
    templatesPath: 'templates',
    assetsPath: 'assets'
});

generator.build();
