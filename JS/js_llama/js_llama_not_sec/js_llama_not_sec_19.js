const emscripten = require('emscripten');
const fs = require('fs');

// Definiere die Funktion, die als WebAssembly-Modul exportiert werden soll
function multiply(a, b) {
  const result = [];
  for (let i = 0; i < a.length; i++) {
    result[i] = a[i] * b[i];
  }
  return result;
}

// Definiere den Emscripten-Code, der die Funktion multiplies implementiert
const emscriptenCode = `
#include <emmintrin.h>
__attribute__((noinline)) void multiply(float* __restrict__ a, float* __restrict__ b, float* 
__restrict__ result) {
  #pragma ivdep
  __m256 v1 = _mm256_loadu_ps(a);
  __m256 v2 = _mm256_loadu_ps(b);
  __m256 v3 = _mm256_mul_ps(v1, v2);
  _mm256_storeu_ps(result, v3);
}
`;

// Erstelle den WebAssembly-Modul-Code
const wasmModuleCode = `
module.exports = {
  multiply: multiply,
};
`;

// Erstelle das Emscripten-Projekt und kompileiere den Code
emscripten.compileSync({
  source: emscriptenCode,
}, (error, mod) => {
  if (error) {
    console.error(error);
    return;
  }

  // Speichere den WebAssembly-Modul-Code in einer Datei
  fs.writeFileSync('multiply.wasm', mod.toString());

  // Lade das WebAssembly-Modul und teste die Funktion
  const wasmModule = require('./multiply.wasm');
  console.log(wasmModule.multiply([1, 2], [3, 4]));
});
const { loadModule } = require('wasmd');

// Lade die WebAssembly-Modul-Datei
loadModule('./multiply.wasm', (error, module) => {
  if (error) {
    console.error(error);
    return;
  }

  // Teste die Funktion
  console.log(module.multiply([1, 2], [3, 4]));
});