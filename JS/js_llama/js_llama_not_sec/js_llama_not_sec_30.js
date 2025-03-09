const { console } = require('console');

// Define eine Funktion für das Conditional Rendering
function ifDirective(name, condition, content) {
    return `<${name}>${condition ? content : ''}</${name}>`;
}

// Define eine Funktion für den Loop
function loopDirective(name, iterable, content) {
    let result = '';
    for (let i = 0; i < iterable.length; i++) {
        const item = iterable[i];
        result += `<item-${i}>
            ${content}
        </item-${i}>`;
    }
    return `<${name}>
        ${result}
    </${name}>`;
}

// Define eine Funktion für die Filters
function filterDirective(name, condition, content) {
    return `<${name}>${condition ? content : ''}</${name}>`;
}

// Define eine Funktion für den Partial-Template-Loader
function partialLoader(templateName, data) {
    const template = require(`./${templateName}`);
    return template.render(data);
}

class TemplateEngine {
    constructor() {}

    render(templateName, data) {
        try {
            const templateContent = this.getTemplateContent(templateName);
            const renderedTemplate = templateContent.render(data);
            console.log(renderedTemplate);
            return renderedTemplate;
        } catch (error) {
            console.error(error);
            throw error;
        }
    }

    getTemplateContent(templateName) {
        const data = require(`./${templateName}`);
        if (!data.render) {
            throw new Error('The template does not have a render method.');
        }
        return data;
    }
}

const templateEngine = new TemplateEngine();

// Define ein Partial-Templat
const exampleData = {
    name: 'John Doe',
};

const exampleTemplateContent = `
  <h1>Hello, ${name}!</h1>
`;

exampleTemplateContent.render = function render(data) {
    const renderedTemplate = this.replace(/name/g, data.name);
    return renderedTemplate;
};
module.exports = { exampleData, exampleTemplateContent };

// Define ein Loops-Templat
const loopData = [
  { id: '1', name: 'John Doe' },
  { id: '2', name: 'Jane Doe' },
];

const loopTemplateContent = `
  <ul>
    ${loop:items ->
      <item-${loop.index}>
        ${name}
      </item-${loop.index}>
    </ul>
  `;

loopTemplateContent.render = function render(data) {
    const renderedTemplate = this.replace(/items/g, data.map((item) => item.id).join(''));
    return renderedTemplate;
};
module.exports = { loopData, loopTemplateContent };

// Define ein If-Templat
const ifData = {
  name: 'John Doe',
};

const ifTemplateContent = `
  ${if:name !== 'John Doe'}
    <p>Hello, Jane!</p>
  ${else}
    <h1>Hello, John Doe!</h1>
  ${endif}
`;

ifTemplateContent.render = function render(data) {
    const renderedTemplate = this.replace(/name/g, data.name);
    return renderedTemplate;
};
module.exports = { ifData, ifTemplateContent };

// Define ein Filter-Templat
const filterData = [
  { id: '1', name: 'John Doe' },
  { id: '2', name: 'Jane Doe' },
];

const filterTemplateContent = `
  ${filter:name === 'John Doe'}
    <p>Hello, John!</p>
  ${else}
    <p>Hello, Jane!</p>
  ${endif}
`;

filterTemplateContent.render = function render(data) {
    const renderedTemplate = this.replace(/name/g, data.name);
    return renderedTemplate;
};
module.exports = { filterData, filterTemplateContent };

// Define ein Loops-Templat mit If
const loopIfData = [
  { id: '1', name: 'John Doe' },
  { id: '2', name: 'Jane Doe' },
];

const loopIfTemplateContent = `
  ${loop:items ->
    <item-${loop.index}>
      ${if:name === 'John Doe'}
        <p>Hello, John!</p>
      ${else}
        <p>Hello, Jane!</p>
      ${endif}
    </item-${loop.index}>
  }
`;

loopIfTemplateContent.render = function render(data) {
    const renderedTemplate = this.replace(/items/g, data.map((item) => 
`<item-${item.id}>`).join(''));
    return renderedTemplate;
};
module.exports = { loopIfData, loopIfTemplateContent };

// Define ein Template mit mehreren Loops
const multipleLoopsData = [
  {
    id: '1',
    name: 'John Doe',
  },
  {
    id: '2',
    name: 'Jane Doe',
  },
];

const multipleLoopsTemplateContent = `
  ${loop:item ->
    <h1>${item.name}</h1>
    ${loop:otherItem ->
      <p>Hello, other item!</p>
    }
  }
`;

multipleLoopsTemplateContent.render = function render(data) {
    const renderedTemplate = this.replace(/item/g, data.map((item) => 
`<item-${item.id}>`).join(''));
    return renderedTemplate;
};
module.exports = { multipleLoopsData, multipleLoopsTemplateContent };

const templateName = 'example';
templateEngine.render(templateName, exampleData);

templateName = 'loop-example';
templateEngine.render(templateName, loopData);

templateName = 'if-example';
templateEngine.render(templateName, ifData);

templateName = 'filter-example';
templateEngine.render(templateName, filterData);

templateName = 'loop-if-example';
templateEngine.render(templateName, loopIfData);

templateName = 'multiple-loops-example';
templateEngine.render(templateName, multipleLoopsData);