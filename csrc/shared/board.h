#pragma once
#include "constants.h"
#include <cassert>
#include <vector>

// offsets for even/odd row neighbors in a pointy-topped layout
// each index is for the corresponding cardinal direction (NE, E, SE, SW, W, NW)
static const int even_row_neighbors[6][2] = {{-1, 0}, {0, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}};
static const int odd_row_neighbors[6][2] = {{-1, 1}, {0, 1}, {1, 1}, {1, 0}, {0, -1}, {-1, 0}};

// we need to able to "move two steps" in a given direction, and compute the
// corresponding direction from two points after the first step from an even
// row, you're at an odd row, so the second step is different but the starting
// location doesn't matter now
static const int double_step_neighbors[6][2] = {{-2, 1}, {0, 2}, {2, 1}, {2, -1}, {0, -2}, {-2, -1}};

inline bool in_bounds(int r, int c) {
    return (r >= 0 && r < ROWS && c >= 0 && c < COLS);
}

inline bool in_bounds(point_t p) {
    return in_bounds(p.first, p.second);
}

static const int min_max_cols[17][2] = {{6, 6},  {5, 6},  {5, 7},  {4, 7},  {0, 12}, {0, 11}, {1, 11}, {1, 10}, {2, 10},
                                        {1, 10}, {1, 11}, {0, 11}, {0, 12}, {4, 7},  {5, 7},  {5, 6},  {6, 6}};

static const point_t player_1_start[10] = {{0, 6}, {1, 5}, {1, 6}, {2, 5}, {2, 6},
                                           {2, 7}, {3, 4}, {3, 5}, {3, 6}, {3, 7}};
static const point_t player_2_start[10] = {{16, 6}, {15, 5}, {15, 6}, {14, 5}, {14, 6},
                                           {14, 7}, {13, 4}, {13, 5}, {13, 6}, {13, 7}};

inline bool is_valid_cell(int r, int c) {
    if (!in_bounds(r, c))
        return false;
    return (c >= min_max_cols[r][0] && c <= min_max_cols[r][1]);
}

inline bool is_valid_cell(point_t p) {
    return is_valid_cell(p.first, p.second);
}

static std::vector<point_t> get_neighbors(point_t p, bool include_invalid = false) {
    // when include_invalid == true, we will always return 6 elements
    std::vector<point_t> result;
    const int(*neighbor_delta)[2] = (p.first % 2 == 0) ? even_row_neighbors : odd_row_neighbors;
    for (int i = 0; i < 6; i++) {
        point_t np = {p.first + neighbor_delta[i][0], p.second + neighbor_delta[i][1]};
        if (include_invalid || (is_valid_cell(np))) {
            result.push_back(np);
        }
    }
    if (include_invalid) {
        assert(result.size() == 6);
    }
    return result;
}

inline char get_neighbors(point_t p, neighbors_t& result, bool include_invalid = false) {
    // when include_invalid == true, we will always return 6 elements
    const int(*neighbor_delta)[2] = (p.first % 2 == 0) ? even_row_neighbors : odd_row_neighbors;
    int count = 0;
    for (int i = 0; i < 6; i++) {
        point_t np = {p.first + neighbor_delta[i][0], p.second + neighbor_delta[i][1]};
        if (include_invalid || (is_valid_cell(np))) {
            result[count++] = np;
        }
    }
    return count;
}

// we want to create a class that will hold the state of the game, but is
// creatable from the PyTorch tensor we'll also want to be able to update the
// state from a move_t
class GameState_t {
public:
    // this is the memory layout of the tensor
    int* grid;
    point_t* player_1_pieces;
    point_t* player_2_pieces;
    int* current_player;
    int* last_skipped_piece;
    int* last_direction;
    int* winner;
    int* turn_count;

    GameState_t(int* flat_cells) {
        // initialize the pointers from a buffer that is owned by the caller
        // use this to keep track of pointers while setting this up
        int* current_ptr = flat_cells;

        // grid is NUM_CELLS ints
        grid = current_ptr;
        current_ptr += NUM_CELLS;

        // player_1_pieces is 10 pairs of ints
        player_1_pieces = reinterpret_cast<point_t*>(current_ptr);
        current_ptr += 10 * sizeof(point_t) / sizeof(int);

        // player_2_pieces is 10 pairs of ints
        player_2_pieces = reinterpret_cast<point_t*>(current_ptr);
        current_ptr += 10 * sizeof(point_t) / sizeof(int);

        current_player = current_ptr;
        current_ptr++;

        last_skipped_piece = current_ptr;
        current_ptr++;

        last_direction = current_ptr;
        current_ptr++;

        winner = current_ptr;
        current_ptr++;

        turn_count = current_ptr;
    }

    inline bool occupied(point_t p) {
        return grid[p.first * COLS + p.second] != EMPTY;
    }

    inline void update_state(point_t from, point_t to, int player, int piece_num) {
        // std::cout << "updating state from: " << from << " to: " << to << " for
        // player " << player << " piece_num "
        // << piece_num << std::endl;
        grid[from.first * COLS + from.second] = EMPTY;
        grid[to.first * COLS + to.second] = player;
        if (player == 1) {
            player_1_pieces[piece_num] = to;
        } else {
            player_2_pieces[piece_num] = to;
        }
    }

    inline void next_turn() {
        // just because we can change the implementation later
        *last_skipped_piece = -1;
        *last_direction = -1;
        // turn 1 to 2, 2 to 1
        *current_player = (*current_player % N_PLAYERS) + 1;
        *turn_count += 1;
    }
};
