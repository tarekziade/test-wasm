#include <emscripten.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>

// Declare an imported function from another WASM module
extern "C" {
    __attribute__((import_module("wasm_gemm"), import_name("onnx_dequantize_linear")))
    void GeckoDequantizeLinear(
        float M, 
        float K, 
        float N, 
        uint32_t input,
        uint32_t scale, 
        uint32_t output, 
        uint32_t zero_point);
}

// Exported function that calls the imported function
extern "C" EMSCRIPTEN_KEEPALIVE void testWasmBuiltin() {
    printf("Calling GeckoDequantizeLinear function...\n");
    
    size_t M = 2, K = 3, N = 4; // Example dimensions
    std::vector<uint8_t> input(M * K * N, 128); // Example input array (uint8_t values)
    std::vector<float> scale(K, 0.1f); // Example scale values
    std::vector<float> output(M * K * N, 0.0f); // Output array
    std::vector<uint8_t> zero_point(K, 0); // Example zero point values

    // Call the imported WASM function
    GeckoDequantizeLinear(
        static_cast<float>(M),
        static_cast<float>(K),
        static_cast<float>(N),
        reinterpret_cast<uint32_t>(input.data()),
        reinterpret_cast<uint32_t>(scale.data()),
        reinterpret_cast<uint32_t>(output.data()),
        reinterpret_cast<uint32_t>(zero_point.data())
    );
    
    printf("GeckoDequantizeLinear function call complete.\n");
}
