// WebAssembly Text Format (WAT) Template für die Vektormultiplikation
const watTemplate = `
(module
  (memory (export "memory") 1)
  (func (export "multiply") (param $ptrA i32) (param $ptrB i32) (param $length i32) (param $ptrResult i32)
    (local $i i32)
    (local.set $i (i32.const 0))
    (block
      (loop
        (br_if 1 (i32.ge_u (local.get $i) (local.get $length)))
        
        ;; Lade Werte aus dem Speicher
        (f64.store
          (i32.add (local.get $ptrResult) (i32.mul (local.get $i) (i32.const 8)))
          (f64.mul
            (f64.load (i32.add (local.get $ptrA) (i32.mul (local.get $i) (i32.const 8))))
            (f64.load (i32.add (local.get $ptrB) (i32.mul (local.get $i) (i32.const 8))))
          )
        )
        
        (local.set $i (i32.add (local.get $i) (i32.const 1)))
        (br 0)
      )
    )
  )
)`;

class WasmVectorMultiplier {
  constructor() {
    this.module = null;
    this.memory = null;
    this.instance = null;
  }

  async initialize() {
    // Konvertiere WAT zu WASM Binärdaten
    const wabt = await import('wabt');
    const module = wabt.parseWat('multiply.wat', watTemplate);
    const binary = module.toBinary({});

    // Kompiliere und instanziiere das WASM Modul
    const wasmModule = await WebAssembly.compile(binary.buffer);
    this.instance = await WebAssembly.instantiate(wasmModule);
    this.memory = this.instance.exports.memory;
  }

  multiply(a, b) {
    if (a.length !== b.length) {
      throw new Error('Arrays müssen die gleiche Länge haben');
    }

    const length = a.length;
    
    // Speicher allozieren
    const bytesPerNumber = 8; // 64-bit float
    const totalBytes = length * bytesPerNumber * 3; // Für Arrays A, B und Ergebnis
    
    if (this.memory.buffer.byteLength < totalBytes) {
      this.memory.grow(Math.ceil(totalBytes / 65536));
    }

    // Arrays in den WASM-Speicher kopieren
    const ptrA = 0;
    const ptrB = length * bytesPerNumber;
    const ptrResult = 2 * length * bytesPerNumber;

    const memoryArrayA = new Float64Array(this.memory.buffer, ptrA, length);
    const memoryArrayB = new Float64Array(this.memory.buffer, ptrB, length);
    
    memoryArrayA.set(a);
    memoryArrayB.set(b);

    // WASM-Funktion aufrufen
    this.instance.exports.multiply(ptrA, ptrB, length, ptrResult);

    // Ergebnis zurückholen
    return Array.from(new Float64Array(this.memory.buffer, ptrResult, length));
  }
}

// Beispielnutzung:
async function example() {
  const multiplier = new WasmVectorMultiplier();
  await multiplier.initialize();

  const a = [1, 2, 3, 4, 5];
  const b = [2, 3, 4, 5, 6];
  
  console.log('Input A:', a);
  console.log('Input B:', b);
  console.log('Result:', multiplier.multiply(a, b));
}

export default WasmVectorMultiplier;