#include <pybind11/pybind11.h>
#include <torch/extension.h>

// Include your shared headers
#include "../shared/board.h"
#include "../shared/chinese_checkers.h"
#include "../shared/constants.h"

torch::Tensor generate_zeros(int64_t x, int64_t y) {
  return torch::zeros({x, y});
}

// Example binding (adapt to your actual functions)
PYBIND11_MODULE(TORCH_EXTENSION_NAME, m) {
  m.def("initialize_board", &initialize_board,
        "Initialize the Chinese Checkers board");
  // Add more bindings as needed
  m.def("generate_zeros", &generate_zeros,
        "Generate a tensor of zeros with given dimensions");
}