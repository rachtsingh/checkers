#pragma once
#include <functional>
#include <unordered_set>
#include <utility>

typedef std::pair<int, int> point_t;

static const size_t N_PLAYERS = 2;
static const size_t N_PIECES_PER_PLAYER = 10;
static const size_t N_DIRECTIONS = 6;
static const size_t ROWS = 17;
static const size_t COLS = 13;
static const size_t NUM_CELLS = ROWS * COLS;
static const size_t GAME_METADATA = 5;
static const size_t TOTAL_STATE =
    NUM_CELLS + N_PLAYERS * N_PIECES_PER_PLAYER * sizeof(point_t) / sizeof(int) + GAME_METADATA;
static const size_t N_MOVES = N_DIRECTIONS * N_PIECES_PER_PLAYER + 1;

static const int EMPTY = 0;
static const int INVALID = -1;
static const float SCALE = 1.00f;
static const float HEX_RADIUS = 30.0f * SCALE;
static const float CIRCLE_RADIUS = 12.0f * SCALE;
static const float MARGIN_X = 50.0f * SCALE;
static const float MARGIN_Y = 50.0f * SCALE;
static const bool SHOW_COORDS = true;

struct pair_hash {
    std::size_t operator()(point_t const& p) const noexcept {
        std::size_t h1 = std::hash<int>{}(p.first);
        std::size_t h2 = std::hash<int>{}(p.second);
        return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
    }
};
