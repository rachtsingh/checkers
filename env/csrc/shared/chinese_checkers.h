#pragma once
#include "board.h"
#include "constants.h"
#include <torch/torch.h>

void initialize_state(GameState_t game_state);
torch::Tensor initialize_state_batched(int n_batch);

void set_action_mask(GameState_t game_state, int* dest);
torch::Tensor get_action_mask_batched(torch::Tensor& game_state_batch, int n_batch);

void update_state(GameState_t game_state, size_t move);
void update_state_batched(torch::Tensor& game_state_batch, torch::Tensor& moves_batch, int n_batch);
