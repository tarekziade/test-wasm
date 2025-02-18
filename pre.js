/**
* Patches the original one so we can inject mozIntGemm and do a single compilation
* 
* getWasmImports() gets called in the main thread, then twice in each em-thread
* The first call is done in createWasm(), and the second via Module["instantiateWasm"]
* On the first call, the thread's wasmMemory variable is not initialized yet,
* on the second call we can hook the import
*/
getWasmImports = function() {
  assignWasmImports();
  if (wasmMemory) {
    const gemmModule = WebAssembly["mozIntGemm"];
    var gemmModuleExports = new WebAssembly.Instance(gemmModule(), {
      "": {
        memory: wasmMemory
      }
    }).exports;
    return {
      "env": wasmImports,
      "wasi_snapshot_preview1": wasmImports,
      "wasm_gemm": gemmModuleExports
    };
  } else {
    return {
      "env": wasmImports,
      "wasi_snapshot_preview1": wasmImports,
    };
  }
}

Module["instantiateWasm"] = async (info, receiveInstance) => {
  const wasmBinaryFile = findWasmBinary();
  const bytes = await getBinaryPromise(wasmBinaryFile);
  const module = await WebAssembly.compile(bytes);
  try {
    var instance = new WebAssembly.Instance(module, getWasmImports());
    receiveInstance(instance, module);  // passing the module so threads can reuse it
  } catch (error) {
    console.error("Error creating WebAssembly instance:", error);
    throw error;
  }
};
