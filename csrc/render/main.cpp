#include "raylib.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "../shared/board.h"
#include "../shared/chinese_checkers.h"
#include "../shared/constants.h"

static Color forest_green = {34, 139, 34, 255};
static Color deep_sea_blue = {0, 105, 148, 255};

static const int window_width = static_cast<int>(2 * MARGIN_X + (COLS - 1) * (sqrtf(3.0f) * HEX_RADIUS) + 0.5f * (sqrtf(3.0f) * HEX_RADIUS));
static const int window_height = static_cast<int>(2 * MARGIN_Y + (ROWS - 1) * (1.5f * HEX_RADIUS));

struct parsed_move_t {
    int player;
    bool end_turn;
    int piece_index;
    int direction;
};

static bool parse_move_line(const std::string& line, parsed_move_t& out_move) {
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string t;
    while (iss >> t) { tokens.push_back(t); }
    if (tokens.size() < 4) return false;
    if (tokens[0] != "PLAYER" || tokens[2] != "MOVE:") return false;
    out_move.player = std::stoi(tokens[1]);
    if (tokens[3] == "END") {
        out_move.end_turn = true;
        out_move.piece_index = -1;
        out_move.direction = -1;
        return true;
    }
    if (tokens.size() < 5) return false;
    out_move.end_turn = false;
    out_move.piece_index = std::stoi(tokens[3]);
    out_move.direction = std::stoi(tokens[4]);
    return true;
}


static Vector2 get_center(int row, int col) {
    float offset_x = (row % 2 == 1) ? (sqrtf(3.0f) * HEX_RADIUS * 0.5f) : 0.0f;
    float center_x = MARGIN_X + offset_x + col * (sqrtf(3.0f) * HEX_RADIUS);
    float center_y = MARGIN_Y + row * (1.5f * HEX_RADIUS);
    return {center_x, center_y};
}

static void render_grid(int* grid) {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (!is_valid_cell(r, c)) continue;
            Vector2 center = get_center(r, c);
            if (grid[r * COLS + c] == 1)
                DrawCircleV(center, CIRCLE_RADIUS, forest_green);
            else if (grid[r * COLS + c] == 2)
                DrawCircleV(center, CIRCLE_RADIUS, deep_sea_blue);
            else
                DrawCircleV(center, CIRCLE_RADIUS, GRAY);
        }
    }
}


static int to_update_state_move_index(const parsed_move_t& move) {
    if (move.end_turn)
        return N_MOVES - 1;
    return move.piece_index * N_DIRECTIONS + move.direction;
}

int main() {
    std::vector<parsed_move_t> move_list;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        parsed_move_t parsed;
        if (parse_move_line(line, parsed))
            move_list.push_back(parsed);
        else
            std::cerr << "ignoring invalid line: " << line << "\n";
    }

    auto states_tensor = initialize_state_batched(1);
    GameState_t game_state(states_tensor[0].data_ptr<int>());

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);
    InitWindow(window_width, window_height, "chinese checkers replay");
    SetTargetFPS(60);

    float elapsed_time = 0.0f;
    int current_move = 0;
    int n_moves = move_list.size();
    bool done = false;

    float time_between_updates = 1.0f;

    while (!WindowShouldClose() && !done) {
        float delta_time = GetFrameTime();
        elapsed_time += delta_time;
        if (elapsed_time >= time_between_updates && current_move < n_moves) {
            auto move = move_list[current_move];
            int move_idx = -1;
            if (move.end_turn)
            {
                move_idx = N_MOVES - 1;
            }
            else
            {
                move_idx = move.piece_index * N_DIRECTIONS + move.direction;
            }
            update_state(game_state, move_idx);
            current_move++;
            elapsed_time = 0.0f;
        }
        if (current_move >= n_moves)
            done = true;

        BeginDrawing();
        ClearBackground(RAYWHITE);
        render_grid(game_state.grid);
        DrawText(TextFormat("Move %d", *game_state.turn_count), 20, 20, 20, DARKGRAY);
        if (done)
            DrawText("DONE. Press ESC to close.", 20, 50, 20, DARKGRAY);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
