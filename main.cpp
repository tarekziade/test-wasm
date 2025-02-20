#include <random>
#include <stdint.h>
#include <stdio.h>
#include <vector>

using namespace std::chrono_literals;

#ifdef ENABLE_LOG
#define LOG(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__);
#else
#define LOG(fmt, ...)
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

// Declare an imported function from another WASM module
extern "C" {
__attribute__((import_module("wasm_gemm"),
               import_name("onnx_dequantize_linear"))) void
GeckoDequantizeLinear(float M, float K, float N, uint32_t input, uint32_t scale,
                      uint32_t output, uint32_t zero_point);
}
#endif

// Reference implementation of DequantizeLinear
void DequantizeLinearRef(size_t M, size_t K, size_t N, const uint8_t *input,
                         float *scale, float *output,
                         const uint8_t *zero_point) {
  for (size_t m = 0; m < M; m++) {
    for (size_t k = 0; k < K; k++) {
      int32_t zp = zero_point ? static_cast<int32_t>(zero_point[k]) : 0;
      for (size_t n = 0; n < N; n += 1) {
        output[m * K * N + k * N + n] =
            float(input[m * K * N + k * N + n] - zp) * scale[k];
      }
    }
  }
}

template <typename T> T random_in_range(T lower, T upper) {
  static std::random_device rd;
  static std::mt19937 gen(rd());

  if constexpr (std::is_integral_v<T>) {
    std::uniform_int_distribution<T> dist(lower, upper);
    return dist(gen);
  } else if constexpr (std::is_floating_point_v<T>) {
    std::uniform_real_distribution<double> dist(static_cast<double>(lower),
                                                static_cast<double>(upper));
    return static_cast<T>(dist(gen));
  }
}

void test_one(const size_t M, const size_t K, const size_t N) {
  std::vector<uint8_t> input(M * K * N);
  std::vector<float> scale(K);
  std::vector<float> output1(M * K * N);
  std::vector<float> output2(M * K * N);
  std::vector<uint8_t> zero_point(K, 0);

  // page the memory in by walking and writing stuff in the input buffers
  std::generate(input.begin(), input.end(),
                [&]() { return random_in_range(0, 255); });
  std::generate(scale.begin(), scale.end(),
                [&]() { return random_in_range(0., 1.0); });
  // output1 and output2 are allocated, not initialized, not paged in.
  // zero_point is always 0

  // Don't always have super hot buffers
  // auto duration = std::chrono::milliseconds(random_in_range(0, 1000));
  // std::this_thread::sleep_for(duration);

#ifdef __EMSCRIPTEN__
  EM_ASM(performance.mark("start_ref"););
#endif
  LOG("Ref\n");
  DequantizeLinearRef(M, K, N, input.data(), scale.data(), output1.data(),
                      zero_point.data());
  LOG("End ref\n");
#ifdef __EMSCRIPTEN__
  EM_ASM(performance.mark("end_ref");
         performance.measure("ReferenceDuration", "start_ref", "end_ref"););
#endif

#ifdef __EMSCRIPTEN__
  EM_ASM(performance.mark("start_imported"););
  LOG("Opt\n");
  GeckoDequantizeLinear(static_cast<float>(M), static_cast<float>(K),
                        static_cast<float>(N),
                        reinterpret_cast<uint32_t>(input.data()),
                        reinterpret_cast<uint32_t>(scale.data()),
                        reinterpret_cast<uint32_t>(output2.data()),
                        reinterpret_cast<uint32_t>(zero_point.data()));
  LOG("End opt\n");
  EM_ASM(performance.mark("end_imported"); performance.measure(
      "ImportedDuration", "start_imported", "end_imported"););
#endif
}

// Exported function that calls the imported function and compares performance
extern "C" void testWasmBuiltin() {
  std::srand(std::time(nullptr));
  for (uint32_t i = 0; i < 30; i++) {
    size_t M = 256, K = 1, N = 1024;
    for (size_t m = 1; m < 2048; m *= 2) {
      LOG("M=%zu K=%zu N=%zu\n", m, K, N);
#ifdef __EMSCRIPTEN__
      EM_ASM(performance.mark("start_run"););
#endif
      test_one(m, K, N);
#ifdef __EMSCRIPTEN__
      EM_ASM(performance.mark("end_run");
             performance.measure(`Run : N = ${$0} K = ${$1} N = ${$2}`,
                                 "start_run", "end_run");
             , m, K, N);
#endif
    }
  }
}

#ifndef __EMSCRIPTEN__
int main() {
  testWasmBuiltin();
  return 0;
}
#endif
