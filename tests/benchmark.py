#!/usr/bin/env python3
import time
import random
import numpy as np
import torch
import click
from chinese_checkers_ext import (
    initialize_state_batched,
    get_action_mask_batched,
    update_state_batched,
    N_MOVES,
)

# Python-only implementation - will be much slower
class PyGameState:
    def __init__(self, rows=17, cols=13, n_pieces=10, n_directions=6):
        self.rows = rows
        self.cols = cols
        self.n_pieces = n_pieces
        self.n_directions = n_directions
        self.grid = np.zeros((rows, cols), dtype=np.int32)
        self.player_1_pieces = []
        self.player_2_pieces = []
        self.current_player = 1
        self.last_skipped_piece = -1
        self.last_direction = -1
        self.winner = 0
        self.turn_count = 0
        
        # Initialize the board
        self._initialize_board()
    
    def _initialize_board(self):
        # Define player 1 starting positions
        player_1_start = [
            (0, 6), (1, 5), (1, 6), (2, 5), (2, 6), (2, 7), (3, 4), (3, 5), (3, 6), (3, 7)
        ]
        
        # Define player 2 starting positions
        player_2_start = [
            (16, 6), (15, 5), (15, 6), (14, 5), (14, 6), (14, 7), (13, 4), (13, 5), (13, 6), (13, 7)
        ]
        
        # Define valid cells
        min_max_cols = [
            (6, 6), (5, 6), (5, 7), (4, 7), (0, 12), (0, 11), (1, 11), (1, 10), (2, 10),
            (1, 10), (1, 11), (0, 11), (0, 12), (4, 7), (5, 7), (5, 6), (6, 6)
        ]
        
        # Initialize the grid
        for r in range(self.rows):
            for c in range(self.cols):
                is_valid = c >= min_max_cols[r][0] and c <= min_max_cols[r][1]
                if not is_valid:
                    self.grid[r, c] = -1  # INVALID
                elif r <= 3 and is_valid:
                    self.grid[r, c] = 1   # PLAYER1
                elif r >= 13 and is_valid:
                    self.grid[r, c] = 2   # PLAYER2
                else:
                    self.grid[r, c] = 0   # EMPTY
        
        # Initialize player pieces
        self.player_1_pieces = player_1_start.copy()
        self.player_2_pieces = player_2_start.copy()
    
    def is_valid_cell(self, r, c):
        if r < 0 or r >= self.rows or c < 0 or c >= self.cols:
            return False
        return self.grid[r, c] != -1
    
    def get_neighbors(self, r, c):
        # Define offsets for even/odd row neighbors
        even_row_neighbors = [(-1, 0), (0, 1), (1, 0), (1, -1), (0, -1), (-1, -1)]
        odd_row_neighbors = [(-1, 1), (0, 1), (1, 1), (1, 0), (0, -1), (-1, 0)]
        
        # Choose the right offset based on whether the row is even or odd
        offsets = even_row_neighbors if r % 2 == 0 else odd_row_neighbors
        
        # Generate all neighbors
        neighbors = []
        for dr, dc in offsets:
            nr, nc = r + dr, c + dc
            if self.is_valid_cell(nr, nc):
                neighbors.append((nr, nc))
            else:
                neighbors.append(None)  # For invalid cells
        
        return neighbors
    
    def get_double_step_neighbors(self, r, c):
        # Define offsets for double step neighbors
        double_step_offsets = [(-2, 1), (0, 2), (2, 1), (2, -1), (0, -2), (-2, -1)]
        
        # Generate all double step neighbors
        double_neighbors = []
        for dr, dc in double_step_offsets:
            nr, nc = r + dr, c + dc
            if self.is_valid_cell(nr, nc):
                double_neighbors.append((nr, nc))
            else:
                double_neighbors.append(None)  # For invalid cells
        
        return double_neighbors
    
    def get_action_mask(self):
        # Create an action mask of the appropriate size
        action_mask = np.zeros(self.n_pieces * self.n_directions + 1, dtype=np.int32)
        
        # Determine if we're in a skipping state
        skip_move = self.last_skipped_piece != -1
        
        # Get the pieces for the current player
        pieces = self.player_1_pieces if self.current_player == 1 else self.player_2_pieces
        
        for i in range(self.n_pieces):
            # If we're in a skipping state, we can only move the last skipped piece
            if skip_move and i != self.last_skipped_piece:
                continue
            
            r, c = pieces[i]
            neighbors = self.get_neighbors(r, c)
            double_neighbors = self.get_double_step_neighbors(r, c)
            
            for j in range(self.n_directions):
                dest_idx = i * self.n_directions + j
                
                # Skip invalid directions
                if neighbors[j] is None:
                    continue
                
                nr, nc = neighbors[j]
                
                # If the next cell is unoccupied and we haven't started skipping,
                # we can make a 1-step move
                if self.grid[nr, nc] == 0 and not skip_move:
                    action_mask[dest_idx] = 1
                elif self.grid[nr, nc] != 0:
                    # The cell is occupied, check if we can jump over it
                    if double_neighbors[j] is None:
                        continue
                    
                    dr, dc = double_neighbors[j]
                    
                    # Check if the landing spot is valid and empty
                    if self.is_valid_cell(dr, dc) and self.grid[dr, dc] == 0:
                        # If we're skipping, prevent moving backward
                        if skip_move and i == self.last_skipped_piece and (self.last_direction + 3) % 6 == j:
                            continue
                        
                        action_mask[dest_idx] = 1
        
        # If we're in a skipping state, we can end the turn
        if skip_move:
            action_mask[-1] = 1
        
        return action_mask
    
    def update_state(self, move):
        # Check if this is an END_MOVE action
        if move == self.n_pieces * self.n_directions:
            # End skipping, reset and switch players
            self.last_skipped_piece = -1
            self.last_direction = -1
            self.current_player = 2 if self.current_player == 1 else 1
            self.turn_count += 1
            return
        
        # Decode the move
        piece_num = move // self.n_directions
        direction = move % self.n_directions
        
        # Get the piece coordinates
        pieces = self.player_1_pieces if self.current_player == 1 else self.player_2_pieces
        r, c = pieces[piece_num]
        
        # Get neighbors and double neighbors
        neighbors = self.get_neighbors(r, c)
        double_neighbors = self.get_double_step_neighbors(r, c)
        
        # Get the potential one-step neighbor
        if neighbors[direction] is None:
            # This shouldn't happen with a valid move
            raise ValueError(f"Invalid move: direction {direction} from {r},{c}")
        
        nr, nc = neighbors[direction]
        
        # Check if we're doing a 1-step or 2-step move
        if self.grid[nr, nc] == 0:
            # 1-step move
            self.grid[r, c] = 0  # Empty the original cell
            self.grid[nr, nc] = self.current_player  # Move to the new cell
            pieces[piece_num] = (nr, nc)  # Update the piece list
            
            # End the turn
            assert self.last_skipped_piece == -1, "Cannot make a 1-step move after skipping"
            self.current_player = 2 if self.current_player == 1 else 1
            self.turn_count += 1
        else:
            # 2-step move (jump)
            if double_neighbors[direction] is None:
                # This shouldn't happen with a valid move
                raise ValueError(f"Invalid jump: direction {direction} from {r},{c}")
            
            dr, dc = double_neighbors[direction]
            
            # Make sure the landing spot is empty
            assert self.grid[dr, dc] == 0, f"Cannot jump to an occupied cell: {dr},{dc}"
            
            # Update the grid and piece position
            self.grid[r, c] = 0  # Empty the original cell
            self.grid[dr, dc] = self.current_player  # Move to the landing cell
            pieces[piece_num] = (dr, dc)  # Update the piece list
            
            # Set the skipping state
            self.last_skipped_piece = piece_num
            self.last_direction = direction

def random_game_py(game_state, num_moves):
    """Play a random game using the pure Python implementation."""
    for _ in range(num_moves):
        # Get the action mask
        action_mask = game_state.get_action_mask()
        
        # Get the valid actions
        valid_actions = np.where(action_mask == 1)[0]
        
        # If there are no valid actions, we're done
        if len(valid_actions) == 0:
            break
        
        # Choose a random valid action
        action = random.choice(valid_actions)
        
        # Update the state
        game_state.update_state(action)

def random_game_cpp(game_state_tensor, num_moves):
    """Play a random game using the C++ extension."""
    for _ in range(num_moves):
        # Get the action mask
        action_mask = get_action_mask_batched(game_state_tensor, 1)
        
        # Get the valid actions
        valid_actions = torch.where(action_mask[0] == 1)[0]
        
        # If there are no valid actions, we're done
        if len(valid_actions) == 0:
            break
        
        # Choose a random valid action
        action_idx = random.randint(0, len(valid_actions) - 1)
        action = valid_actions[action_idx].item()
        
        # Create a tensor for the action
        action_tensor = torch.tensor([action], dtype=torch.int32)
        
        # Update the state
        update_state_batched(game_state_tensor, action_tensor, 1)

@click.command()
@click.option('--num-games', default=100, help='Number of games to simulate')
@click.option('--max-moves', default=200, help='Maximum number of moves per game')
def benchmark(num_games, max_moves):
    """Benchmark the Python and C++ implementations."""
    print(f"Benchmarking with {num_games} games, max {max_moves} moves per game...")
    
    # Benchmark the Python implementation
    start_time = time.time()
    for _ in range(num_games):
        game_state = PyGameState()
        random_game_py(game_state, max_moves)
    py_time = time.time() - start_time
    
    # Benchmark the C++ implementation
    start_time = time.time()
    for _ in range(num_games):
        game_state_tensor = initialize_state_batched(1)
        random_game_cpp(game_state_tensor, max_moves)
    cpp_time = time.time() - start_time
    
    # Print the results
    print(f"Python implementation: {py_time:.4f} seconds")
    print(f"C++ implementation: {cpp_time:.4f} seconds")
    print(f"Speedup: {py_time / cpp_time:.2f}x")

if __name__ == "__main__":
    benchmark()