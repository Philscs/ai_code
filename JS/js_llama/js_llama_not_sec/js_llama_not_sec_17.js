// Importiere die benötigten Bibliotheken
import { html, litElement } from 'lit-element';

// Definiere eine Funktion zum Erstellen eines neuen Elements
function createComponent(componentDefinition) {
  // Entlese den Tagname und die Style des Komponenten aus der Definition
  const { tag, style } = componentDefinition;

  // Erstelle das Element mit dem angegebenen Tagname und Style
  return litElement(`[${tag}]`, {
    style: JSON.stringify(style),
  });
}

// Definiere eine Funktion zum Erstellen eines neuen Elements mit Slot-Content
function createSlotComponent(componentDefinition) {
  // Entlese den Tagname, die Style und den Slot-Text aus der Definition
  const { tag, style, slot } = componentDefinition;

  // Erstelle das Element mit dem angegebenen Tagname und Style
  return litElement(`[${tag}]`, {
    style: JSON.stringify(style),
    template: html`
      <div class="card">
        ${slot}
      </div>
    `,
  });
}

// Definiere die Komponente-Definition für den Card-Component
const cardDefinition = {
  tag: 'custom-card',
  style: `
    :host { display: block; }
  `,
};

// Erstelle den Card-Element mit dem Slot-Content
const cardElement = createSlotComponent(cardDefinition);

export default cardElement;
