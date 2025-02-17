#!/usr/bin/env python3
import os
import copy
import math
import pathlib
import sys
from dataclasses import dataclass
import click
import parse
from rich import print
import numpy as np
import pygame
from chinese_checkers_ext import (
    odd_row_neighbors,
    even_row_neighbors,
    double_step_neighbors,
    ROWS,
    COLS,
    N_PIECES_PER_PLAYER,
    N_DIRECTIONS,
    N_MOVES,
    TOTAL_STATE,
    MIN_MAX_COLS,
)

os.environ["SDL_VIDEO_HIGHDPI"] = "1"
INVALID = -1
EMPTY = 0
PLAYER1 = 1
PLAYER2 = 2

@dataclass
class Point:
    x: int
    y: int
    def shift(self, dr: int, dc: int) -> "Point":
        return Point(self.x + dr, self.y + dc)
    def move(self, direction: int) -> "Point":
        offsets = even_row_neighbors if self.x % 2 == 0 else odd_row_neighbors
        dr, dc = offsets[direction]
        return self.shift(dr, dc)
    def move_double(self, direction: int) -> "Point":
        dr, dc = double_step_neighbors[direction]
        return self.shift(dr, dc)
    @classmethod
    def from_tuple(cls, tup: tuple[int, int]) -> "Point":
        return cls(tup[0], tup[1])

player_1_start = [Point(0, 6), Point(1, 5), Point(1, 6), Point(2, 5), Point(2, 6),
                  Point(2, 7), Point(3, 4), Point(3, 5), Point(3, 6), Point(3, 7)]
player_2_start = [Point(16, 6), Point(15, 5), Point(15, 6), Point(14, 5), Point(14, 6),
                  Point(14, 7), Point(13, 4), Point(13, 5), Point(13, 6), Point(13, 7)]

@dataclass
class GameState:
    grid: np.ndarray
    player_1_pieces: list[Point]
    player_2_pieces: list[Point]
    last_move: tuple[int, int, Point, Point] | None
    current_player: int = 1
    last_skipped_piece: int = -1
    last_direction: int = -1
    winner: int = 0
    turn_count: int = 0
    def __init__(self):
        self.grid = np.zeros((ROWS, COLS), dtype=np.int32)
        self.player_1_pieces = copy.copy(player_1_start)
        self.player_2_pieces = copy.copy(player_2_start)
        self.current_player = 1
    def occupied(self, r: int, c: int) -> bool:
        return self.grid[r, c] > EMPTY
    def occupied_p(self, p: Point) -> bool:
        return self.occupied(p.x, p.y)
    def update_piece(self, from_rc: Point, to_rc: Point, player: int, piece_index: int):
        self.grid[from_rc.x, from_rc.y] = EMPTY
        self.grid[to_rc.x, to_rc.y] = player
        if player == PLAYER1:
            self.player_1_pieces[piece_index] = copy.copy(to_rc)
        else:
            self.player_2_pieces[piece_index] = copy.copy(to_rc)
    def next_turn(self):
        self.last_skipped_piece = -1
        self.last_direction = -1
        self.current_player = 1 if self.current_player == 2 else 2
        self.turn_count += 1

def is_valid_cell(r: int, c: int) -> bool:
    if not (0 <= r < ROWS and 0 <= c < COLS):
        return False
    return MIN_MAX_COLS[r][0] <= c <= MIN_MAX_COLS[r][1]

def is_valid_cell_p(p: Point) -> bool:
    return is_valid_cell(p.x, p.y)

def initialize_state() -> GameState:
    gs = GameState()
    for r in range(ROWS):
        for c in range(COLS):
            if (r <= 3) and is_valid_cell(r, c):
                gs.grid[r, c] = PLAYER1
            elif (r >= 13) and is_valid_cell(r, c):
                gs.grid[r, c] = PLAYER2
            if not is_valid_cell(r, c):
                gs.grid[r, c] = INVALID
    return gs

@dataclass
class ParsedMove:
    player: int
    end_turn: bool
    piece_num: int | None
    direction: int | None

def parse_move_line(line: str) -> ParsedMove:
    if p := parse.parse("PLAYER {:d} MOVE: {:d} {:d}", line):
        return ParsedMove(player=p[0], end_turn=False, piece_num=p[1], direction=p[2])
    if q := parse.parse("PLAYER {:d} MOVE: END TURN", line):
        return ParsedMove(player=q[0], end_turn=True, piece_num=None, direction=None)
    raise ValueError(f"Line does not match expected patterns: {line}")

def update_state(gs: GameState, piece_num: int, direction: int, end_turn: bool):
    if end_turn:
        gs.next_turn()
        return
    cur_player = gs.current_player
    piece = gs.player_1_pieces[piece_num] if cur_player == PLAYER1 else gs.player_2_pieces[piece_num]
    one_step = piece.move(direction)
    two_step = piece.move_double(direction)
    if is_valid_cell_p(one_step) and gs.occupied_p(one_step):
        if not is_valid_cell_p(two_step) or gs.occupied_p(two_step):
            raise ValueError("Invalid jump")
        gs.update_piece(piece, two_step, cur_player, piece_num)
        gs.last_skipped_piece = piece_num
        gs.last_direction = direction
        gs.last_move = (cur_player, piece_num, piece, two_step)
    else:
        if not is_valid_cell_p(one_step) or gs.occupied_p(one_step):
            raise ValueError("Invalid move")
        gs.update_piece(piece, one_step, cur_player, piece_num)
        if gs.last_skipped_piece != -1:
            raise ValueError("Invalid skip logic")
        gs.next_turn()
        gs.last_move = (cur_player, piece_num, piece, one_step)

def draw_hex_grid(screen: pygame.Surface, gs: GameState, font: pygame.font.Font | None):
    screen.fill((255, 255, 255))
    for r in range(ROWS):
        for c in range(COLS):
            if gs.grid[r, c] == INVALID:
                continue
            center_x = MARGIN_X + (math.sqrt(3.0) * HEX_RADIUS * 0.5 if r % 2 == 1 else 0) + c * (math.sqrt(3.0) * HEX_RADIUS)
            center_y = MARGIN_Y + r * (1.5 * HEX_RADIUS)
            color = (34,139,34) if gs.grid[r, c] == PLAYER1 else ((0,105,148) if gs.grid[r, c] == PLAYER2 else (200,200,200))
            pygame.draw.circle(screen, color, (int(center_x), int(center_y)), int(0.5 * HEX_RADIUS))
    pygame.display.flip()
    if gs.last_skipped_piece != -1 and gs.last_move is not None:
        fx, fy = MARGIN_X, MARGIN_Y  # simplified placeholder for first piece center
        tx, ty = MARGIN_X, MARGIN_Y  # simplified placeholder for second piece center
        pygame.draw.circle(screen, (255,0,0), (int(fx), int(fy)), int(0.5 * HEX_RADIUS), width=2)
        pygame.draw.circle(screen, (0,255,0), (int(tx), int(ty)), int(0.5 * HEX_RADIUS), width=2)
    if font is not None:
        for idx, point in enumerate(gs.player_1_pieces):
            cx, cy = MARGIN_X, MARGIN_Y  # simplified; compute actual center if needed
            label = font.render(str(idx), True, (0,0,0))
            screen.blit(label, (cx, cy))
        for idx, point in enumerate(gs.player_2_pieces):
            cx, cy = MARGIN_X, MARGIN_Y
            label = font.render(str(idx), True, (0,0,0))
            screen.blit(label, (cx, cy))
    pygame.display.flip()

@click.command()
@click.option("--log", "-i", "log_file", required=True, type=click.Path(path_type=pathlib.Path), help="Path to game log file.")
@click.option("--out", "-o", "out_file", default="final_grid.state", show_default=True, type=click.Path(path_type=pathlib.Path), help="Output grid file.")
@click.option("--render", is_flag=True, help="Visualize moves with PyGame.")
def simulate(log_file: pathlib.Path, out_file: pathlib.Path, render: bool):
    lines = log_file.read_text(encoding="utf-8").splitlines()
    gs = initialize_state()
    screen = None
    if render:
        pygame.init()
        width = int(MARGIN_X*2 + (math.sqrt(3)*HEX_RADIUS)*(COLS-1) + 0.5*(math.sqrt(3)*HEX_RADIUS))
        height = int(MARGIN_Y*2 + (1.5*HEX_RADIUS)*(ROWS-1))
        screen = pygame.display.set_mode((width, height))
        font = pygame.font.SysFont(None, 24)
        pygame.display.set_caption("Chinese Checkers Replay")
    else:
        font = None
    for line in lines:
        line = line.strip()
        if not line: continue
        parsed = parse_move_line(line)
        if parsed.end_turn:
            update_state(gs, 0, 0, True)
        else:
            update_state(gs, parsed.piece_num, parsed.direction, False)
        if render:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    sys.exit(0)
            draw_hex_grid(screen, gs, font)
            pygame.time.wait(1000)
    if render:
        pygame.time.wait(2000)
        pygame.quit()
    with out_file.open("w", encoding="utf-8") as f:
        for r in range(ROWS):
            f.write(" ".join(str(val) for val in gs.grid[r, :].tolist()) + "\n")
    click.secho(f"Final grid written to {out_file}", fg="green")

if __name__ == "__main__":
    simulate()
