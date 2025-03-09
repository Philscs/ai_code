// BEGIN: Include Emscripten
const { instantiate } = require('emscripten');

async function multiply(a, b) {
  const module = await instantiate(fs.readFileSync('path/to/your/compiled/wasm/module.wasm'));

  const multiplyFn = module.instance.exports.multiply;
  const aPtr = module.instance.exports.__retain(module.instance.exports.__allocArray(module.instance.exports.INT32ARRAY_ID, a));
  const bPtr = module.instance.exports.__retain(module.instance.exports.__allocArray(module.instance.exports.INT32ARRAY_ID, b));

  const resultPtr = multiplyFn(aPtr, bPtr);
  const result = module.instance.exports.__getArray(resultPtr);

  module.instance.exports.__release(aPtr);
  module.instance.exports.__release(bPtr);
  module.instance.exports.__release(resultPtr);

  return result;
}
// END: Include Emscripten
