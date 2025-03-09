class WebComponent extends HTMLElement {
    constructor() {
        super();
        this.attachShadow({ mode: "open" });
    }

    initialize(templateString, styles = "") {
        const template = document.createElement("template");
        template.innerHTML = `
            <style>${styles}</style>
            ${this.sanitizeHTML(templateString)}
        `;
        this.shadowRoot.appendChild(template.content.cloneNode(true));
    }

    sanitizeHTML(html) {
        const div = document.createElement("div");
        div.textContent = html;
        return div.innerHTML;
    }
}

class ComponentRegistry {
    static register(name, componentClass) {
        if (!customElements.get(name)) {
            customElements.define(name, componentClass);
        } else {
            console.warn(`The element "${name}" is already registered.`);
        }
    }
}

class MyButton extends WebComponent {
    constructor() {
        super();

        const template = `
            <button part="button">
                <slot></slot>
            </button>
        `;

        const styles = `
            button {
                background-color: #007BFF;
                color: white;
                padding: 0.5em 1em;
                border: none;
                border-radius: 4px;
                cursor: pointer;
                font-size: 1em;
            }

            button:hover {
                background-color: #0056b3;
            }
        `;

        this.initialize(template, styles);
    }

    connectedCallback() {
        this.shadowRoot.querySelector("button").addEventListener("click", () => {
            this.dispatchEvent(new CustomEvent("button-click", {
                bubbles: true,
                composed: true
            }));
        });
    }

    disconnectedCallback() {
        this.shadowRoot.querySelector("button").removeEventListener("click", this.handleClick);
    }
}

ComponentRegistry.register("my-button", MyButton);
