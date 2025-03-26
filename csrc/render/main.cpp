#include "../shared/board.h"
#include "../shared/chinese_checkers.h"
#include "../shared/constants.h"
#include "raylib.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static Color forest_green = {34, 139, 34, 255};
static Color deep_sea_blue = {0, 105, 148, 255};
static Color move_arrow_color = {255, 69, 0, 200}; // Orange-red with some transparency

static const int window_width =
    static_cast<int>(2 * MARGIN_X + (COLS - 1) * (sqrtf(3.0f) * HEX_RADIUS) + 0.5f * (sqrtf(3.0f) * HEX_RADIUS));
static const int window_height = static_cast<int>(2 * MARGIN_Y + (ROWS - 1) * (1.5f * HEX_RADIUS));

struct parsed_move_t {
    int player;
    bool end_turn;
    int piece_index;
    int direction;
    point_t from_pos;
    point_t to_pos;
};

static bool parse_move_line(const std::string& line, parsed_move_t& out_move) {
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string t;
    while (iss >> t) {
        tokens.push_back(t);
    }
    if (tokens.size() < 4)
        return false;
    if (tokens[0] != "PLAYER" || tokens[2] != "MOVE:")
        return false;
    out_move.player = std::stoi(tokens[1]);

    // Check for "END TURN" format (tokens[3] == "END" && tokens[4] == "TURN")
    if (tokens[3] == "END" && tokens.size() >= 5 && tokens[4] == "TURN") {
        out_move.end_turn = true;
        out_move.piece_index = -1;
        out_move.direction = -1;
        // For END moves, we don't have from/to positions
        out_move.from_pos = {-1, -1};
        out_move.to_pos = {-1, -1};
        return true;
    }

    if (tokens.size() < 5)
        return false;
    out_move.end_turn = false;
    out_move.piece_index = std::stoi(tokens[3]);
    out_move.direction = std::stoi(tokens[4]);
    // We'll set the positions later when processing the moves
    out_move.from_pos = {-1, -1};
    out_move.to_pos = {-1, -1};
    return true;
}

static Vector2 get_center(int row, int col) {
    float offset_x = (row % 2 == 1) ? (sqrtf(3.0f) * HEX_RADIUS * 0.5f) : 0.0f;
    float center_x = MARGIN_X + offset_x + col * (sqrtf(3.0f) * HEX_RADIUS);
    float center_y = MARGIN_Y + row * (1.5f * HEX_RADIUS);
    return {center_x, center_y};
}

static void render_grid(GameState_t& game_state, bool show_grid_indices = false,
                        const parsed_move_t* last_move = nullptr) {
    int* grid = game_state.grid;

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (!is_valid_cell(r, c))
                continue;
            auto center = get_center(r, c);
            if (grid[r * COLS + c] == 1)
                DrawCircleV(center, CIRCLE_RADIUS, forest_green);
            else if (grid[r * COLS + c] == 2)
                DrawCircleV(center, CIRCLE_RADIUS, deep_sea_blue);
            else
                DrawCircleV(center, CIRCLE_RADIUS, GRAY);

            if (show_grid_indices) {
                auto text = TextFormat("%d,%d", r, c);
                auto text_width = MeasureText(text, 10);
                DrawText(text, center.x - text_width / 2, center.y + CIRCLE_RADIUS + 2, 10, DARKGRAY);
            }
        }
    }

    if (last_move && last_move->from_pos.first != -1 && last_move->to_pos.first != -1) {
        auto start = get_center(last_move->from_pos.first, last_move->from_pos.second);
        auto end = get_center(last_move->to_pos.first, last_move->to_pos.second);
        DrawLineEx(start, end, 3.0f, move_arrow_color);
    }

    for (int i = 0; i < N_PIECES_PER_PLAYER; i++) {
        auto piece = game_state.player_1_pieces[i];
        auto center = get_center(piece.first, piece.second);
        auto text_width = (i == 1) ? 2 : 6;
        DrawText(TextFormat("%d", i), center.x - text_width / 2, center.y - 6, 12, BLACK);
    }

    for (int i = 0; i < N_PIECES_PER_PLAYER; i++) {
        auto piece = game_state.player_2_pieces[i];
        auto center = get_center(piece.first, piece.second);
        auto text_width = (i == 1) ? 2 : 6;
        DrawText(TextFormat("%d", i), center.x - text_width / 2, center.y - 6, 12, WHITE);
    }
}

int main(int argc, char** argv) {
    bool show_grid_indices = false;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--show-grid-indices" || std::string(argv[i]) == "-g") {
            show_grid_indices = true;
        }
    }

    std::vector<parsed_move_t> raw_move_list;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty())
            continue;
        parsed_move_t parsed;
        if (parse_move_line(line, parsed))
            raw_move_list.push_back(parsed);
        else
            std::cerr << "ignoring invalid line: " << line << "\n";
    }

    // Process the raw moves to group submoves and set from/to positions
    std::vector<parsed_move_t> move_list;

    auto states_tensor = initialize_state_batched(1);
    GameState_t game_state(states_tensor[0].data_ptr<int>());

    for (size_t i = 0; i < raw_move_list.size(); i++) {
        auto move = raw_move_list[i];

        // Handle END_TURN moves properly
        if (move.end_turn) {
            // Add the END_TURN move explicitly to the move list
            // so the update_state will properly handle the player change
            move_list.push_back(move);

            // Update the game state with the END_TURN move
            update_state(game_state, N_MOVES - 1);
            continue;
        }

        // Get the current piece position before the move
        auto player = move.player;
        auto piece_num = move.piece_index;
        auto piece = (player == 1) ? game_state.player_1_pieces[piece_num] : game_state.player_2_pieces[piece_num];

        // Store original position
        move.from_pos = piece;

        // Calculate destination based on move direction
        neighbors_t neighbors;
        get_neighbors(piece, neighbors, true);
        auto one_step = neighbors[move.direction];

        // Determine if it's a one-step or a skip move
        if (!game_state.occupied(one_step)) {
            // One-step move
            move.to_pos = one_step;
        } else {
            // Skip move
            auto offset = double_step_neighbors[move.direction];
            point_t two_step = {piece.first + offset[0], piece.second + offset[1]};
            move.to_pos = two_step;
        }

        // Add to processed move list
        move_list.push_back(move);

        // Update the game state to reflect this move
        int move_idx = -1;
        if (move.end_turn) {
            move_idx = N_MOVES - 1;
        } else {
            move_idx = move.piece_index * N_DIRECTIONS + move.direction;
        }
        update_state(game_state, move_idx);
    }

    // Reset the game state for the actual rendering
    states_tensor = initialize_state_batched(1);
    game_state = GameState_t(states_tensor[0].data_ptr<int>());

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);
    InitWindow(window_width, window_height, "chinese checkers replay");
    SetTargetFPS(60);

    float elapsed_time = 0.0f;
    int current_move = 0;
    int n_moves = move_list.size();
    bool done = false;
    parsed_move_t* last_move = nullptr;

    // Different timing for different types of moves
    float regular_move_time = 1.0f; // Regular moves
    float skip_move_time = 0.5f;    // Faster for multi-hop submoves
    float end_turn_time = 0.3f;     // Even faster for END TURN moves
    float time_between_updates = regular_move_time;

    while (!WindowShouldClose() && !done) {
        float delta_time = GetFrameTime();
        elapsed_time += delta_time;
        if (elapsed_time >= time_between_updates && current_move < n_moves) {
            auto move = move_list[current_move];
            int move_idx = -1;
            if (move.end_turn) {
                move_idx = N_MOVES - 1;
                // Set timing for the next move (after END TURN)
                time_between_updates = regular_move_time;
            } else {
                move_idx = move.piece_index * N_DIRECTIONS + move.direction;

                // Determine if this is part of a skip sequence
                bool is_skip = false;
                if (current_move < n_moves - 1) {
                    // Look ahead to see if next move is by same player (skip sequence)
                    if (move_list[current_move + 1].player == move.player && !move_list[current_move + 1].end_turn) {
                        is_skip = true;
                        // Set faster timing for the next move in this skip sequence
                        time_between_updates = skip_move_time;
                    } else if (move_list[current_move + 1].end_turn) {
                        // Next is END TURN, set faster timing
                        time_between_updates = end_turn_time;
                    } else {
                        // Regular move follows
                        time_between_updates = regular_move_time;
                    }
                }
            }
            update_state(game_state, move_idx);
            last_move = &move_list[current_move];
            current_move++;
            elapsed_time = 0.0f;
        }
        if (current_move >= n_moves)
            done = true;

        BeginDrawing();
        ClearBackground(RAYWHITE);
        render_grid(game_state, show_grid_indices, last_move);
        DrawText(TextFormat("Move %d", *game_state.turn_count), 20, 20, 20, DARKGRAY);
        if (done)
            DrawText("DONE. Press ESC to close.", 20, 50, 20, DARKGRAY);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
