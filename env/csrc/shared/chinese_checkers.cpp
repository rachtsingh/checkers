#include "chinese_checkers.h"
#include "board.h"
#include "constants.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <torch/torch.h>

auto static const tensor_options = torch::dtype(torch::kInt32).requires_grad(false);

void initialize_state(GameState_t game_state) {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            int idx = r * COLS + c;
            if ((r <= 3) && is_valid_cell(r, c)) {
                game_state.grid[idx] = 1;
            } else if ((r >= 13) && is_valid_cell(r, c)) {
                game_state.grid[idx] = 2;
            } else if (!is_valid_cell(r, c)) {
                game_state.grid[idx] = INVALID;
            } else {
                game_state.grid[idx] = EMPTY;
            }
        }
    }
    std::copy(player_1_start.begin(), player_1_start.end(), game_state.player_1_pieces);
    std::copy(player_2_start.begin(), player_2_start.end(), game_state.player_2_pieces);
    *game_state.current_player = 1;
    *game_state.last_skipped_piece = -1;
    *game_state.last_direction = -1;
    *game_state.winner = 0;
    *game_state.turn_count = 0;
}

torch::Tensor initialize_state_batched(int n_batch) {
    auto tensor = torch::zeros({n_batch, (long long)TOTAL_STATE}, tensor_options);
    auto tensor_data = tensor.data_ptr<int>();
    for (int i = 0; i < n_batch; i++) {
        auto grid_state = GameState_t(tensor_data + i * TOTAL_STATE);
        initialize_state(grid_state);
    }
    return tensor;
}

void set_action_mask(GameState_t game_state, int* dest) {
    auto player = *game_state.current_player;
    auto last_skipped_piece = *game_state.last_skipped_piece;
    auto skip_move = last_skipped_piece != -1;
    auto last_direction = *game_state.last_direction;

    for (size_t i = 0; i < N_PIECES_PER_PLAYER; i++) {
        if (last_skipped_piece != -1 && last_skipped_piece != i) {
            // we can only move the last skipped piece if a skip has already happened
            continue;
        }

        auto piece = (player == 1) ? game_state.player_1_pieces[i] : game_state.player_2_pieces[i];
        auto neighbors = get_neighbors(piece, true);
        for (int j = 0; j < N_DIRECTIONS; j++) {
            auto dest_idx = i * N_DIRECTIONS + j;
            auto one_step = neighbors[j];
            if (!is_valid_cell(one_step)) {
                continue;
            }
            // if the next cell is unoccupied, the only move we can make is 1 step
            // if we haven't already started skipping
            if (!game_state.occupied(one_step)) {
                if (!skip_move) {
                    dest[dest_idx] = 1;
                }
            } else {
                // now we know that the one step move in that direction is occupied
                // so we are ONLY looking to see if it's possible to make a two step
                // move (i.e. if it's not occupied)
                auto offset = double_step_neighbors[j];
                point_t two_step = {piece.first + offset[0], piece.second + offset[1]};
                if (!is_valid_cell(two_step)) {
                    continue;
                }
                // we have already checked at this point that one_step is occupied
                if (!game_state.occupied(two_step)) {
                    // we mask out the previous step if we've already started skipping
                    // so we can't undo a move
                    if (skip_move && (last_skipped_piece == i) &&
                        (((last_direction - j + N_DIRECTIONS) % N_DIRECTIONS) == 3)) {
                        continue;
                    }
                    dest[dest_idx] = 1;
                }
            }
        }
    }

    // finally, if skip_move is set, we set the last action mask to 1
    if (skip_move) {
        dest[N_MOVES - 1] = 1;
    }
}

torch::Tensor get_action_mask_batched(torch::Tensor& game_state_batch, int n_batch) {
    auto tensor = torch::zeros({n_batch, (long long)N_MOVES}, tensor_options);
    auto tensor_data = tensor.data_ptr<int>();
    game_state_batch = game_state_batch.contiguous();
    auto game_state_batch_ptr = game_state_batch.data_ptr<int>();
    for (int i = 0; i < n_batch; i++) {
        auto grid_state = GameState_t(game_state_batch_ptr + i * TOTAL_STATE);
        auto dest = tensor_data + i * N_MOVES;
        set_action_mask(grid_state, dest);
    }
    return tensor;
}

void update_state(GameState_t game_state, size_t move) {
    auto current_player = *game_state.current_player;
    if (move == N_MOVES - 1) {
        // end skipping, so we reset and switch players
        game_state.next_turn();
        return;
    }

    size_t piece_num = move / 6;
    size_t direction = move % 6;

    // now we need to figure out if we're moving 1 or 2 steps (we assume the move
    // is valid)
    auto piece = (current_player == 1) ? game_state.player_1_pieces[piece_num] : game_state.player_2_pieces[piece_num];
    auto neighbors = get_neighbors(piece, true);
    auto one_step = neighbors[direction];
    auto offset = double_step_neighbors[direction];
    auto two_step = point_t{piece.first + offset[0], piece.second + offset[1]};
    if (!game_state.occupied(one_step)) {
        game_state.update_state(piece, one_step, current_player, piece_num);
        // make sure last_skipped_piece is -1 (should be masked off)
        assert(*game_state.last_skipped_piece == -1);
        game_state.next_turn();
    } else {
        // we know that two_step is empty
        assert(!game_state.occupied(two_step));
        game_state.update_state(piece, two_step, current_player, piece_num);
        // now we set the "last_skipped_piece" flag and DONT switch players
        *game_state.last_skipped_piece = piece_num;
        *game_state.last_direction = direction;
    }
}

void update_state_batched(torch::Tensor& game_state_batch, torch::Tensor& action_batch, int n_batch) {
    game_state_batch = game_state_batch.contiguous();
    action_batch = action_batch.contiguous();
    auto game_state_batch_ptr = game_state_batch.data_ptr<int>();
    auto action_batch_ptr = action_batch.data_ptr<int>();
    for (int i = 0; i < n_batch; i++) {
        auto grid_state = GameState_t(game_state_batch_ptr + i * TOTAL_STATE);
        auto move = action_batch_ptr[i];
        update_state(grid_state, move);
    }
}