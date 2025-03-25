This project implements a mixed Python/C++ project that represents, plays, and renders a
chinese checkers game. It is intended for use in simulation and testing, so it has bindings
to the PyTorch library via torch.h.

There are two main folders: src/ and csrc/, which denote the Python and C++ source respectively. Most of the core game logic is in the C++ files in csrc/shared/, which are used by the 3 C++ targets: generate (in generate/main.cpp), render (in render/main.cpp), and the Python C++ extension (in ext/).

For efficiency, we generally store the game state in a flat buffer, and create GameState objects which are just a collection of pointers into those buffers. This makes it easy to create a BATCH_SIZE x TOTAL_STATE tensor which holds BATCH_SIZE games at once (see initialize_state_batched).

One important note is that in chinese checkers players are allowed to jump over pieces that are adjacent to them, and continue jumping that piece as many times as is possible and desired (without creating an infinite loop, of course). Instead of making the game search logic complicated, we've decided to represent a move like that (which consists of K jumps) using K + 1 "move"s instead, all of which are by the same player: the first K are each individual hop, then the last move is an "END_MOVE" move. This makes the set of possible moves (invalid or valid) simple: 6 directions x 10 pieces + 1 extra for END_MOVE.

The Justfile is used by the just command runner, but might currently contain bugs.