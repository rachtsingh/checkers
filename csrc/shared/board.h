#pragma once
#include "constants.h"
#include <cassert>
#include <vector>
#include <array>

// Offsets for even/odd row neighbors
static const std::array<std::array<int, 2>, 6> even_row_neighbors = {{
    {-1, 0}, {0, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}
}};
static const std::array<std::array<int, 2>, 6> odd_row_neighbors = {{
    {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {0, -1}, {-1, 0}
}};
static const std::array<std::array<int, 2>, 6> double_step_neighbors = {{
    {-2, 1}, {0, 2}, {2, 1}, {2, -1}, {0, -2}, {-2, -1}
}};

inline bool in_bounds(int r, int c) {
    return (r >= 0 && r < ROWS && c >= 0 && c < COLS);
}

typedef std::pair<int, int> point_t;

inline bool in_bounds(point_t p) {
    return in_bounds(p.first, p.second);
}

static const std::array<std::array<int, 2>, 17> min_max_cols = {{
    {6, 6}, {5, 6}, {5, 7}, {4, 7}, {0, 12}, {0, 11}, {1, 11}, {1, 10},
    {2, 10}, {1, 10}, {1, 11}, {0, 11}, {0, 12}, {4, 7}, {5, 7}, {5, 6}, {6, 6}
}};

static const std::array<std::array<int, 2>, 10> player_1_start = {{
    {0, 6}, {1, 5}, {1, 6}, {2, 5}, {2, 6}, {2, 7}, {3, 4}, {3, 5}, {3, 6}, {3, 7}
}};
static const std::array<std::array<int, 2>, 10> player_2_start = {{
    {16, 6}, {15, 5}, {15, 6}, {14, 5}, {14, 6}, {14, 7}, {13, 4}, {13, 5}, {13, 6}, {13, 7}
}};

inline bool is_valid_cell(int r, int c) {
    if (!in_bounds(r, c))
        return false;
    return (c >= min_max_cols[r][0] && c <= min_max_cols[r][1]);
}

inline bool is_valid_cell(point_t p) {
    return is_valid_cell(p.first, p.second);
}

class GameState_t {
public:
    int* grid;
    point_t* player_1_pieces;
    point_t* player_2_pieces;
    int* current_player;
    int* last_skipped_piece;
    int* last_direction;
    int* winner;
    int* turn_count;

    GameState_t(int* flat_cells) {
        int* current_ptr = flat_cells;
        grid = current_ptr;
        current_ptr += ROWS * COLS;
        player_1_pieces = reinterpret_cast<point_t*>(current_ptr);
        current_ptr += 10 * sizeof(point_t) / sizeof(int);
        player_2_pieces = reinterpret_cast<point_t*>(current_ptr);
        current_ptr += 10 * sizeof(point_t) / sizeof(int);
        current_player = current_ptr++;
        last_skipped_piece = current_ptr++;
        last_direction = current_ptr++;
        winner = current_ptr++;
        turn_count = current_ptr;
    }

    inline bool occupied(point_t p) {
        return grid[p.first * COLS + p.second] != EMPTY;
    }

    inline void update_state(point_t from, point_t to, int player, int piece_num) {
        grid[from.first * COLS + from.second] = EMPTY;
        grid[to.first * COLS + to.second] = player;
        if (player == 1) {
            player_1_pieces[piece_num] = to;
        } else {
            player_2_pieces[piece_num] = to;
        }
    }

    inline void next_turn() {
        *last_skipped_piece = -1;
        *last_direction = -1;
        *current_player = (*current_player % 2) + 1;
        *turn_count += 1;
    }
};
