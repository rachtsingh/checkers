#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <torch/torch.h>
#include "../shared/board.h"
#include "../shared/chinese_checkers.h"
#include "../shared/constants.h"
#include <chrono>

static void print_usage() {
    std::cerr << "Usage: ./generate_exe run -n <batch_size> -o <log_file>\n";
    std::exit(1);
}

int32_t sample_allowed_action(const torch::Tensor& action_mask) {
    auto ptr = action_mask.data_ptr<int>();
    int32_t count = 0;
    for (int i = 0; i < N_MOVES; i++) {
        if (ptr[i] == 1) count++;
    }
    int32_t rand_idx = std::rand() % count;
    count = 0;
    for (int i = 0; i < N_MOVES; i++) {
        if (ptr[i] == 1) {
            if (count == rand_idx) return i;
            count++;
        }
    }
    return -1;
}

int main(int argc, char* argv[]) {
    if (argc < 3) print_usage();
    int n = -1;
    std::string log_file;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "run") == 0) continue;
        else if (std::strcmp(argv[i], "-n") == 0 && i + 1 < argc) n = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "-o") == 0 && i + 1 < argc) log_file = argv[++i];
    }
    if (n <= 0) print_usage();

    std::srand((unsigned int)std::time(NULL));
    std::ofstream file_stream;
    std::ostream* out_stream = &std::cout;
    if (!log_file.empty()) {
        file_stream.open(log_file);
        if (!file_stream.is_open()) {
            std::cerr << "Could not open log file: " << log_file << "\n";
            return 1;
        }
        out_stream = &file_stream;
    }

    auto states = initialize_state_batched(1);
    GameState_t game_state(states[0].data_ptr<int>());
    auto start = std::chrono::high_resolution_clock::now();

    while (*game_state.turn_count < n) {
        auto action_mask = get_action_mask_batched(states, 1);
        int chosen_move = sample_allowed_action(action_mask[0]);
        int current_player = *game_state.current_player;
        if (chosen_move == N_MOVES - 1)
            *out_stream << "PLAYER " << current_player << " MOVE: END TURN\n";
        else {
            int piece_num = chosen_move / N_DIRECTIONS;
            int direction = chosen_move % N_DIRECTIONS;
            *out_stream << "PLAYER " << current_player << " MOVE: " << piece_num << " " << direction << "\n";
        }
        update_state(game_state, chosen_move);
        if ((*game_state.turn_count + 1) % 100000 == 0) {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
            std::cerr << "Turn: " << *game_state.turn_count + 1 << " = " << duration.count() << "s\n";
        }
    }
    if (file_stream.is_open()) file_stream.close();
    return 0;
}
