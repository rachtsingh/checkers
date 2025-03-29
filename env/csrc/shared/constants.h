#pragma once

// #include "raylib.h"

#include <array>
#include <functional> // for std::hash<int>
#include <unordered_set>
#include <utility> // for std::pair

// utility class
typedef std::pair<int, int> point_t;

// game constants
static const size_t N_PLAYERS = 2;
static const size_t N_PIECES_PER_PLAYER = 10;
static const size_t N_DIRECTIONS = 6;
static const size_t ROWS = 17;
static const size_t COLS = 13;
static const size_t NUM_CELLS = ROWS * COLS;

// current_player, winner, last_skipped_piece, last_direction, turn_count
static const size_t GAME_METADATA = 5;

// 1 int to store the what is happening in each grid point + 10
static const size_t TOTAL_STATE =
    NUM_CELLS + (N_PLAYERS * N_PIECES_PER_PLAYER * sizeof(point_t) / sizeof(int)) + GAME_METADATA;


    static const size_t N_MOVES = N_DIRECTIONS * N_PIECES_PER_PLAYER + 1;

static const int EMPTY = 0;
static const int INVALID = -1;

// rendering constants, should probably be somewhere else
static const float SCALE = 1.00f;
static const float HEX_RADIUS = 30.0f * SCALE;
static const float CIRCLE_RADIUS = 12.0f * SCALE;
static const float MARGIN_X = 50.0f * SCALE;
static const float MARGIN_Y = 50.0f * SCALE;

static const bool SHOW_COORDS = true;

typedef std::array<point_t, N_DIRECTIONS> neighbors_t;
// formatting methods to allow us to do std::cout << point_t
// inline std::ostream& operator<<(std::ostream& os, const point_t& p) {
//     os << "(" << p.first << ", " << p.second << ")";
//     return os;
// }

struct pair_hash {
    std::size_t operator()(point_t const& p) const noexcept {
        std::size_t h1 = std::hash<int>{}(p.first);
        std::size_t h2 = std::hash<int>{}(p.second);
        return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
    }
};