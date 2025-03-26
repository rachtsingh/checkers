#!/usr/bin/env python3
"""
Validate moves in a Chinese Checkers game log file.

This script reads a game log file and validates each move to identify any
correctness issues in either the move generation or rendering.
"""

import sys
import os
import pathlib
from dataclasses import dataclass
from typing import List, Tuple, Optional

# Add the project root to the path so we can import the Python module
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from src.chinese_checkers_ext import (
    N_DIRECTIONS, 
    N_MOVES, 
    ROWS, 
    COLS, 
    even_row_neighbors,
    odd_row_neighbors,
    double_step_neighbors,
)

# Constants
EMPTY = 0
PLAYER1 = 1
PLAYER2 = 2
INVALID = -1

@dataclass
class Point:
    x: int
    y: int

    def __eq__(self, other):
        if not isinstance(other, Point):
            return False
        return self.x == other.x and self.y == other.y
    
    def __hash__(self):
        return hash((self.x, self.y))
    
    def __str__(self):
        return f"({self.x}, {self.y})"


@dataclass
class Move:
    player: int
    piece_index: int
    direction: int
    end_turn: bool = False
    
    def __str__(self):
        if self.end_turn:
            return f"PLAYER {self.player} MOVE: END TURN"
        return f"PLAYER {self.player} MOVE: {self.piece_index} {self.direction}"


@dataclass
class GameState:
    grid: List[List[int]]
    player_1_pieces: List[Point]
    player_2_pieces: List[Point]
    current_player: int
    last_skipped_piece: int
    last_direction: int
    turn_count: int
    
    def __init__(self):
        # Initialize grid
        self.grid = [[EMPTY for _ in range(COLS)] for _ in range(ROWS)]
        
        # Initialize player pieces
        self.player_1_pieces = [
            Point(0, 6), Point(1, 5), Point(1, 6), 
            Point(2, 5), Point(2, 6), Point(2, 7),
            Point(3, 4), Point(3, 5), Point(3, 6), Point(3, 7)
        ]
        
        self.player_2_pieces = [
            Point(16, 6), Point(15, 5), Point(15, 6), 
            Point(14, 5), Point(14, 6), Point(14, 7),
            Point(13, 4), Point(13, 5), Point(13, 6), Point(13, 7)
        ]
        
        # Set initial grid state
        for p in self.player_1_pieces:
            self.grid[p.x][p.y] = PLAYER1
            
        for p in self.player_2_pieces:
            self.grid[p.x][p.y] = PLAYER2
        
        # Mark invalid cells
        for r in range(ROWS):
            for c in range(COLS):
                if not is_valid_cell(r, c):
                    self.grid[r][c] = INVALID
        
        # Initialize game state
        self.current_player = 1
        self.last_skipped_piece = -1
        self.last_direction = -1
        self.turn_count = 0
    
    def occupied(self, p: Point) -> bool:
        """Check if a cell is occupied."""
        if not is_valid_cell(p.x, p.y):
            return False
        return self.grid[p.x][p.y] != EMPTY
    
    def get_neighbors(self, p: Point) -> List[Point]:
        """Get all neighbors of a point"""
        neighbors = []
        if p.x % 2 == 0:  # Even row
            for dr, dc in even_row_neighbors:
                nx, ny = p.x + dr, p.y + dc
                if is_valid_cell(nx, ny):
                    neighbors.append(Point(nx, ny))
        else:  # Odd row
            for dr, dc in odd_row_neighbors:
                nx, ny = p.x + dr, p.y + dc
                if is_valid_cell(nx, ny):
                    neighbors.append(Point(nx, ny))
        return neighbors
    
    def next_turn(self):
        """Switch to the next player's turn."""
        self.last_skipped_piece = -1
        self.last_direction = -1
        self.current_player = 3 - self.current_player  # Toggle between 1 and 2
        self.turn_count += 1


def is_valid_cell(r: int, c: int) -> bool:
    """Check if a cell is valid on the board."""
    if r < 0 or r >= ROWS or c < 0 or c >= COLS:
        return False
    
    # Use the min_max_cols to determine if this is a valid cell
    min_max_cols = [
        [6, 6], [5, 6], [5, 7], [4, 7], [0, 12], [0, 11], [1, 11],
        [1, 10], [2, 10], [1, 10], [1, 11], [0, 11], [0, 12], [4, 7],
        [5, 7], [5, 6], [6, 6]
    ]
    min_col, max_col = min_max_cols[r]
    return min_col <= c <= max_col


def parse_move(line: str) -> Optional[Move]:
    """Parse a move from a log line."""
    line = line.strip()
    if not line:
        return None
        
    if "END TURN" in line:
        parts = line.split()
        if len(parts) == 4 and parts[0] == "PLAYER" and parts[2] == "MOVE:":
            return Move(int(parts[1]), -1, -1, end_turn=True)
        return None
        
    parts = line.split()
    if len(parts) == 5 and parts[0] == "PLAYER" and parts[2] == "MOVE:":
        return Move(int(parts[1]), int(parts[3]), int(parts[4]))
    
    return None


def get_next_position(piece: Point, direction: int) -> Tuple[Point, bool]:
    """
    Get the next position after moving in a direction.
    Returns the new position and whether it's a jump.
    """
    # Get the neighbor offset based on even/odd row
    if piece.x % 2 == 0:  # Even row
        dr, dc = even_row_neighbors[direction]
    else:  # Odd row
        dr, dc = odd_row_neighbors[direction]
    
    # One step neighbor
    one_step = Point(piece.x + dr, piece.y + dc)
    
    # Two step neighbor (for jumps)
    dr, dc = double_step_neighbors[direction]
    two_step = Point(piece.x + dr, piece.y + dc)
    
    return one_step, two_step


def validate_move(game_state: GameState, move: Move) -> Tuple[bool, str]:
    """
    Validate a move and update the game state if valid.
    Returns (is_valid, error_message)
    """
    if move.end_turn:
        # END_MOVE is only valid if we're in the middle of a skip sequence
        if game_state.last_skipped_piece == -1:
            return False, "END_MOVE when not in a skip sequence"
        
        # Update game state for end turn
        game_state.next_turn()
        return True, ""
    
    # Check if player is correct
    if move.player != game_state.current_player:
        return False, f"Wrong player: expected {game_state.current_player}, got {move.player}"
    
    # Check if piece index is valid
    if move.piece_index < 0 or move.piece_index >= 10:
        return False, f"Invalid piece index: {move.piece_index}"
    
    # Check if we're in a skip sequence and this is a different piece
    if game_state.last_skipped_piece != -1 and game_state.last_skipped_piece != move.piece_index:
        return False, f"Cannot move piece {move.piece_index} when in middle of skip sequence with piece {game_state.last_skipped_piece}"
    
    # Get the current piece position
    if game_state.current_player == 1:
        piece = game_state.player_1_pieces[move.piece_index]
    else:
        piece = game_state.player_2_pieces[move.piece_index]
    
    # Check if the direction is valid
    if move.direction < 0 or move.direction >= N_DIRECTIONS:
        return False, f"Invalid direction: {move.direction}"
    
    # Get the next positions
    one_step, two_step = get_next_position(piece, move.direction)
    
    # Check if we can move in this direction
    if not is_valid_cell(one_step.x, one_step.y):
        return False, f"Invalid move direction: {move.direction} leads off the board"
        
    # If one step is empty, it's a regular move
    if not game_state.occupied(one_step):
        # Cannot make a regular move if we're in a skip sequence
        if game_state.last_skipped_piece != -1:
            return False, "Cannot make a regular move during a skip sequence"
        
        # Update the grid and piece position
        game_state.grid[piece.x][piece.y] = EMPTY
        game_state.grid[one_step.x][one_step.y] = game_state.current_player
        
        # Update the piece position
        if game_state.current_player == 1:
            game_state.player_1_pieces[move.piece_index] = one_step
        else:
            game_state.player_2_pieces[move.piece_index] = one_step
        
        # Switch to next player
        game_state.next_turn()
        
    # If one step is occupied, check if we can jump
    else:
        # Check if the two-step position is valid and empty
        if not is_valid_cell(two_step.x, two_step.y):
            return False, f"Jump destination off board: {two_step}"
        
        if game_state.occupied(two_step):
            return False, f"Jump destination occupied: {two_step}"
        
        # Check if we're trying to jump back to where we came from
        if game_state.last_skipped_piece == move.piece_index:
            if ((move.direction + 3) % 6) == game_state.last_direction:
                return False, "Cannot jump back to the previous position"
            
        # Update the grid and piece position for a jump
        game_state.grid[piece.x][piece.y] = EMPTY
        game_state.grid[two_step.x][two_step.y] = game_state.current_player
        
        # Update the piece position
        if game_state.current_player == 1:
            game_state.player_1_pieces[move.piece_index] = two_step
        else:
            game_state.player_2_pieces[move.piece_index] = two_step
        
        # Update skip state
        game_state.last_skipped_piece = move.piece_index
        game_state.last_direction = move.direction
    
    return True, ""


def validate_log_file(log_path: str) -> List[Tuple[int, str, str]]:
    """
    Validate all moves in a game log file.
    Returns a list of errors with line number, move, and error message.
    """
    with open(log_path, 'r') as f:
        lines = f.readlines()
    
    errors = []
    game_state = GameState()
    
    for i, line in enumerate(lines):
        move = parse_move(line)
        if not move:
            errors.append((i+1, line.strip(), "Could not parse move"))
            continue
        
        valid, error_msg = validate_move(game_state, move)
        if not valid:
            errors.append((i+1, line.strip(), error_msg))
    
    return errors


def analyze_piece_distribution(log_path: str):
    """Analyze how many times each piece is moved in the game."""
    with open(log_path, 'r') as f:
        lines = f.readlines()
    
    player1_piece_counts = [0] * 10
    player2_piece_counts = [0] * 10
    
    for line in lines:
        move = parse_move(line)
        if move and not move.end_turn:
            if move.player == 1:
                player1_piece_counts[move.piece_index] += 1
            else:
                player2_piece_counts[move.piece_index] += 1
    
    print("\nPiece Movement Analysis:")
    print("Player 1 piece movements:")
    for i, count in enumerate(player1_piece_counts):
        print(f"  Piece {i}: {count} moves")
    
    print("Player 2 piece movements:")
    for i, count in enumerate(player2_piece_counts):
        print(f"  Piece {i}: {count} moves")


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <game_log_file>")
        sys.exit(1)
    
    log_path = sys.argv[1]
    if not os.path.exists(log_path):
        print(f"Error: File '{log_path}' does not exist.")
        sys.exit(1)
    
    print(f"Validating game log: {log_path}")
    errors = validate_log_file(log_path)
    
    if not errors:
        print("✅ All moves are valid!")
    else:
        print(f"❌ Found {len(errors)} errors:")
        for line_num, move, error in errors:
            print(f"  Line {line_num}: {move} - {error}")
    
    analyze_piece_distribution(log_path)


if __name__ == "__main__":
    main()