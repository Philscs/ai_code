import { styled } from 'styled-components';

// Scoped Styles
const Button = styled.button`
  background-color: #4CAF50;
  color: #fff;
  padding: 10px 20px;
  border-radius: 5px;
  cursor: pointer;

  &:hover {
    background-color: #3e8e41;
  }
`;

// Dynamic Theming
function getButtonStyles(theme) {
  return theme === 'light' ? Button`
    background-color: #fff;
    color: #000;
  ` : Button`
    background-color: #333;
    color: #fff;
  `;
}

// Style Composition
const composeStyles = (style1, style2) => `
  ${style1}
  , ${style2}
`;

// Critical CSS Extraction
function extractCriticalStyles(style1, style2) {
  const criticalStyles = [
    'background-color',
    'color',
    'padding',
    'border-radius',
    'cursor',
  ];

  return style1.split(' {').map((part) => part.trim()).filter((part) => 
criticalStyles.includes(part));
}

// Runtime Style Injection
function injectStyle(theme) {
  const style = `
    ${getButtonStyles(theme)}
  `;

  document.head.insertAdjacentHTML('beforeend', `<style>${style}</style>`);
}

const theme = 'dark'; // Beispiel-Wert für den thematischen Stil

injectStyle(theme);

// Output:
// <button background-color="#333" color="#fff" padding="10px 20px" border-radius="5px" cursor:"pointer">
//   Hello Welt!
// </button>