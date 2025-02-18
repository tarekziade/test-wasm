#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <chrono>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

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
#endif

// Reference implementation of DequantizeLinear
void DequantizeLinearRef(
    size_t M, size_t K, size_t N, const uint8_t* input,
    float* scale, float* output, const uint8_t* zero_point)
{
  for (size_t m = 0; m < M; m++) {
    for (size_t k = 0; k < K; k++) {
      int32_t zp = zero_point ? static_cast<int32_t>(zero_point[k]) : 0;
      for (size_t n = 0; n < N; n+=1) {
        output[m * K * N + k * N + n] = float(input[m * K * N + k * N + n] - zp) * scale[k];
      }
    }
  }
}

// Exported function that calls the imported function and compares performance
extern "C" void testWasmBuiltin() {
    printf("Calling DequantizeLinearRef function...\n");
    
    size_t M = 256, K = 1, N = 1024; 
    std::vector<uint8_t> input(M * K * N, 0);
    std::vector<float> scale(K, 0.1f); 
    std::vector<float> output1(M * K * N, 0.0f);
    std::vector<float> output2(M * K * N, 0.0f); 
    std::vector<uint8_t> zero_point(K, 0); 

    // Measure time for reference function
    auto start_ref = std::chrono::high_resolution_clock::now();
    DequantizeLinearRef(M, K, N, input.data(), scale.data(), output1.data(), zero_point.data());
    auto end_ref = std::chrono::high_resolution_clock::now();
    auto duration_ref = std::chrono::duration_cast<std::chrono::nanoseconds>(end_ref - start_ref).count();

#ifdef __EMSCRIPTEN__
    // Measure time for imported WASM function
    auto start_wasm = std::chrono::high_resolution_clock::now();
    GeckoDequantizeLinear(
        static_cast<float>(M),
        static_cast<float>(K),
        static_cast<float>(N),
        reinterpret_cast<uint32_t>(input.data()),
        reinterpret_cast<uint32_t>(scale.data()),
        reinterpret_cast<uint32_t>(output2.data()),
        reinterpret_cast<uint32_t>(zero_point.data())
    );
    auto end_wasm = std::chrono::high_resolution_clock::now();
    auto duration_wasm = std::chrono::duration_cast<std::chrono::nanoseconds>(end_wasm - start_wasm).count();

    printf("WASM function time: %lld ns\n", duration_wasm);
#endif
    
    printf("Reference function time: %lld ns\n", duration_ref);
}

#ifndef __EMSCRIPTEN__
int main() {
    testWasmBuiltin();
    return 0;
}
#endif
