//===- GoogleBenchmarkMain.cpp---------------------------------------------===//
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//===----------------------------------------------------------------------===//
//
// This file implements the benchmark for reduce maxf operation.
//
//===----------------------------------------------------------------------===//

#include <benchmark/benchmark.h>
#include <buddy/Core/Container.h>
#include <iostream>
#include <random>

// Define target layout.
#define INPUT_N 1
#define INPUT_H 32
#define INPUT_W 32
#define INPUT_C 16
#define OUTPUT_N 1
#define OUTPUT_H 32
#define OUTPUT_W 32
#define OUTPUT_C 16

// Helper functions and variables.
namespace {
const std::string PASS = "\033[32mPASS\033[0m";
const std::string FAIL = "\033[31mFAIL\033[0m";

bool areArraysEqual(float array1[], float array2[], int size) {
  for (int i = 0; i < size; ++i) {
    if (array1[i] != array2[i]) {
      return false;
    }
  }
  return true;
}
} // namespace

namespace {
// Declare the math.maxf C interface.
extern "C" {
void _mlir_ciface_maxf_scalar(MemRef<float, 4> *input1, MemRef<float, 4> *input2, MemRef<float, 4> *output);
void _mlir_ciface_maxf_auto_vectorization(MemRef<float, 4> *input1, MemRef<float, 4> *input2, MemRef<float, 4> *output);
}

#define DEFINE_MAXF_BENCHMARK(name, func)                                     \
  void BM_MAXF_##name(benchmark::State &state) {                              \
    intptr_t sizesInput[4] = {INPUT_N, INPUT_H, INPUT_W, INPUT_C};            \
    intptr_t sizesOutput[4] = {OUTPUT_N, OUTPUT_H, OUTPUT_W, OUTPUT_C};       \
                                                                              \
    MemRef<float, 4> input1(sizesInput, 1.0);                                 \
    MemRef<float, 4> input2(sizesInput, 1.0);                                 \
    MemRef<float, 4> output(sizesOutput, 0.0);                                \
                                                                              \
    for (auto _ : state) {                                                    \
      func(&input1, &input2, &output);                                        \
    }                                                                         \
  }

DEFINE_MAXF_BENCHMARK(SCALAR, _mlir_ciface_maxf_scalar)
DEFINE_MAXF_BENCHMARK(AutoVectorization, _mlir_ciface_maxf_auto_vectorization)
} // namespace

// Register benchmark cases.
BENCHMARK(BM_MAXF_SCALAR)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_MAXF_AutoVectorization)->Unit(benchmark::kMillisecond);

/// Correctness Verification
/// The verification does not affect the performance.
/// - Set the scalar case as the criteria.
/// - Input elements are random numbers.
/// - Output elements are initialized to zero.
/// - Compare the output of various optimizations with the scalar version to
///   verify correctness.
void verification() {
  // Set the random number generator.
  std::random_device rd;
  std::mt19937 generator(rd());
  std::uniform_real_distribution<float> distribution(0.0, 1.0);

  // Set the layout sizes of input and output memref container.
  intptr_t sizesInput[4] = {INPUT_N, INPUT_H, INPUT_W, INPUT_C};
  intptr_t sizesOutput[4] = {OUTPUT_N, OUTPUT_H, OUTPUT_W, OUTPUT_C};

  // Generate input memref containers with random numbers.
  const int inputSize = INPUT_N * INPUT_H * INPUT_W * INPUT_C;
  float inputRand1[inputSize];
  float inputRand2[inputSize];
  for (int i = 0; i < inputSize; ++i) {
    inputRand1[i] = distribution(generator);
    inputRand2[i] = distribution(generator);
  }
  MemRef<float, 4> inputMemRef1(inputRand1, sizesInput);
  MemRef<float, 4> inputMemRef2(inputRand2, sizesInput);

  // Generate output memref containers with zero.
  const int outputSize = OUTPUT_N * OUTPUT_H * OUTPUT_W * OUTPUT_C;
  MemRef<float, 4> outputScalar(sizesOutput, 0.0);
  MemRef<float, 4> outputAutoVectorization(sizesOutput, 0.0);

  // Perform all the maxf implementations.
  _mlir_ciface_maxf_scalar(&inputMemRef1, &inputMemRef2, &outputScalar);
  _mlir_ciface_maxf_auto_vectorization(&inputMemRef1, &inputMemRef2, &outputAutoVectorization);

  // Get the result array.
  auto resultScalar = outputScalar.getData();
  auto resultAutoVectorization = outputAutoVectorization.getData();

  // Print the verification result.
  std::cout << "-----------------------------------------------------------"
            << std::endl;
  std::cout << "Correctness Verification:" << std::endl;
  std::cout << "Transform case: "
            << (areArraysEqual(resultScalar, resultAutoVectorization,
                               outputSize)
                    ? PASS
                    : FAIL)
            << std::endl;
  std::cout << "-----------------------------------------------------------"
            << std::endl;
}

int main(int argc, char **argv) {
  // Run benchmark.
  ::benchmark::Initialize(&argc, argv);
  ::benchmark::RunSpecifiedBenchmarks();
  // Run correctness verification.
  verification();
  return 0;
}
