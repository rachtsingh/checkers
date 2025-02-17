#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <torch/extension.h>
#include "../shared/board.h"
#include "../shared/chinese_checkers.h"
#include "../shared/constants.h"

namespace py = pybind11;

torch::Tensor init_state_batched_wrap(int64_t n_batch) {
    return initialize_state_batched((int)n_batch);
}

torch::Tensor get_action_mask_batched_wrap(torch::Tensor game_state_batch) {
    auto n_batch = game_state_batch.size(0);
    return get_action_mask_batched(game_state_batch, (int)n_batch);
}

int64_t update_state_batched_wrap(torch::Tensor game_state_batch, torch::Tensor moves_batch) {
    auto n_batch = game_state_batch.size(0);
    update_state_batched(game_state_batch, moves_batch, (int)n_batch);
    return 0;
}

PYBIND11_MODULE(TORCH_EXTENSION_NAME, m) {
    m.attr("ROWS") = ROWS;
    m.attr("COLS") = COLS;
    m.attr("N_PIECES_PER_PLAYER") = N_PIECES_PER_PLAYER;
    m.attr("N_DIRECTIONS") = N_DIRECTIONS;
    m.attr("N_MOVES") = N_MOVES;
    m.attr("TOTAL_STATE") = TOTAL_STATE;

    m.attr("even_row_neighbors") = py::cast(even_row_neighbors);
    m.attr("odd_row_neighbors") = py::cast(odd_row_neighbors);
    m.attr("double_step_neighbors") = py::cast(double_step_neighbors);
    m.attr("min_max_cols") = py::cast(min_max_cols);
    m.attr("player_1_start") = py::cast(player_1_start);
    m.attr("player_2_start") = py::cast(player_2_start);

    m.def("initialize_state_batched", &init_state_batched_wrap,
          "Create n_batch new game states (batched, CPU).");
    m.def("get_action_mask_batched", &get_action_mask_batched_wrap,
          "Get action mask for batched game states.");
    m.def("update_state_batched", &update_state_batched_wrap,
          "Update batched game states in-place.");
}

// Register custom ops if needed
TORCH_LIBRARY(chinese_checkers_ext, m) {
    m.def("initialize_state_batched(int n_batch) -> Tensor");
    m.def("get_action_mask_batched(Tensor game_state_batch) -> Tensor");
    m.def("update_state_batched(Tensor game_state_batch, Tensor moves_batch) -> int");
}

TORCH_LIBRARY_IMPL(chinese_checkers_ext, CPU, m) {
    m.impl("initialize_state_batched", &init_state_batched_wrap);
    m.impl("get_action_mask_batched", &get_action_mask_batched_wrap);
    m.impl("update_state_batched", &update_state_batched_wrap);
}
