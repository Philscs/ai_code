// Importiere die `fs`-Bibliothek für Dateisystemzugriffe (nur für Node.js erforderlich)
const fs = require("fs");
const path = require("path");

async function compileWasm() {
  // AssemblyScript-Quellcode als String definieren
  const ascSource = `
  export function multiply(a: Float64Array, b: Float64Array, size: i32): void {
    for (let i: i32 = 0; i < size; i++) {
      a[i] = a[i] * b[i];
    }
  }
  `;

  // AssemblyScript in eine temporäre Datei schreiben
  const ascFilePath = path.resolve(__dirname, "temp-multiply.ts");
  fs.writeFileSync(ascFilePath, ascSource);

  // AssemblyScript mit dem `asc`-Compiler in WebAssembly übersetzen
  const { execSync } = require("child_process");
  execSync(`npx asc ${ascFilePath} --outFile temp-multiply.wasm --optimize`);

  // WebAssembly-Datei lesen
  const wasmBuffer = fs.readFileSync(path.resolve(__dirname, "temp-multiply.wasm"));
  return wasmBuffer;
}

async function loadWasm(wasmBuffer) {
  const { instance } = await WebAssembly.instantiate(wasmBuffer, {});
  return instance.exports;
}

(async () => {
  // Schritt 1: WebAssembly erstellen
  const wasmBuffer = await compileWasm();

  // Schritt 2: WebAssembly laden
  const wasm = await loadWasm(wasmBuffer);

  // Schritt 3: Speicher und Daten vorbereiten
  const memory = new WebAssembly.Memory({ initial: 1 });
  const array = new Float64Array(memory.buffer, 0, 20);

  // Daten für Arrays einfügen
  array.set([1, 2, 3, 4, 5], 0); // Array `a`
  array.set([5, 4, 3, 2, 1], 10); // Array `b`

  // WebAssembly-Funktion aufrufen
  wasm.multiply(0, 10, 5); // (Offset von `a`, Offset von `b`, Größe der Arrays)

  // Ergebnis auslesen
  const result = array.subarray(0, 5); // Array `a` enthält jetzt die Ergebnisse
  console.log("Result:", Array.from(result)); // Ausgabe: [5, 8, 9, 8, 5]

  // Bereinige temporäre Dateien
  fs.unlinkSync("temp-multiply.ts");
  fs.unlinkSync("temp-multiply.wasm");
})();
