

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <torch/torch.h>

// include local render.cpp file and constants.h file
#include "../shared/board.h"
#include "../shared/chinese_checkers.h"
#include "../shared/constants.h"

static void print_usage() {
    std::cerr << "Usage: ./chinese-checkers run -n <batch_size> -o <log_file>" << std::endl;
    std::exit(1);
}

// int32_t sample_allowed_action(const torch::Tensor& action_mask) {
//     auto allowed = action_mask.nonzero();
//     auto rand_idx = torch::randint(0, allowed.size(0), {1}).item<int32_t>();
//     return allowed[rand_idx].item<int32_t>();
// }

// note: will need to rewrite this for batched version
int32_t sample_allowed_action(const torch::Tensor& action_mask) {
    // we want to implement the above without all the aten checks, so using action_mask.data_ptr<int>() directly
    // we'll also assume that the action_mask is 1D
    auto ptr = action_mask.data_ptr<int>();
    int32_t count = 0;
    for (int i = 0; i < N_MOVES; i++) {
        if (ptr[i] == 1) {
            count++;
        }
    }
    int32_t rand_idx = std::rand() % count;
    count = 0;
    for (int i = 0; i < N_MOVES; i++) {
        if (ptr[i] == 1) {
            if (count == rand_idx) {
                return i;
            }
            count++;
        }
    }
    // should never reach here
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage();
    }

    // basic arg parsing: ./chinese-checkers run -n 1 -o game_trace.log
    int n = -1;
    std::string log_file;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "run") == 0) {
            // do nothing; just confirm subcommand
        } else if (std::strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            n = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            log_file = argv[++i];
        }
    }
    if (n <= 0) {
        print_usage();
    }

    // seed RNG for different output
    std::srand((unsigned int)std::time(NULL));
    // std::srand(0);

    // decide where to log (file or std::cout)
    std::ofstream file_stream;
    std::ostream* out_stream = &std::cout; // default to stdout
    if (!log_file.empty()) {
        file_stream.open(log_file);
        if (!file_stream.is_open()) {
            std::cerr << "Could not open log file: " << log_file << std::endl;
            return 1;
        }
        out_stream = &file_stream;
    }

    // initialize states for batch size n
    auto states = initialize_state_batched(1);

    auto game_state = GameState_t(states[0].data_ptr<int>());

    // start time
    auto start = std::chrono::high_resolution_clock::now();

    while (*game_state.turn_count < n) {
        // get valid moves
        auto action_mask = get_action_mask_batched(states.data_ptr<int>(), 1);
        auto chosen_move = sample_allowed_action(action_mask[0]);
        free_underlying_buffer(action_mask);

        // write move to log
        int current_player = *game_state.current_player;
        if (chosen_move == N_MOVES - 1) {
            // END TURN move
            *out_stream << "PLAYER " << current_player << " MOVE: END TURN\n";
        } else {
            int piece_num = chosen_move / N_DIRECTIONS;
            int direction = chosen_move % N_DIRECTIONS;
            *out_stream << "PLAYER " << current_player << " MOVE: " << piece_num << " " << direction << "\n";
        }

        // apply move to game state
        update_state(game_state, chosen_move);

        // Optional: check if we have a winner
        // if (*game_state.winner != 0) {
        //     logStream << "PLAYER " << *game_state.winner << " WINS!\n";
        //     break;
        // }
        // print out the timing every 100k moves
        if ((*game_state.turn_count + 1) % 100000 == 0) {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
            std::cerr << "Total time elapsed for turn: " << *game_state.turn_count + 1 << " = " << duration.count()
                      << "s\n";
        }
    }

    if (file_stream.is_open()) {
        file_stream.close();
    }
    return 0;
}
