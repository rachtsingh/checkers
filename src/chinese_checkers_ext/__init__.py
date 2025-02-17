import torch
from . import _C as c_ext
from typing import Callable

even_row_neighbors = c_ext.even_row_neighbors
odd_row_neighbors = c_ext.odd_row_neighbors
double_step_neighbors = c_ext.double_step_neighbors
ROWS = c_ext.ROWS
COLS = c_ext.COLS
N_PIECES_PER_PLAYER = c_ext.N_PIECES_PER_PLAYER
N_DIRECTIONS = c_ext.N_DIRECTIONS
N_MOVES = c_ext.N_MOVES
TOTAL_STATE = c_ext.TOTAL_STATE
MIN_MAX_COLS = c_ext.min_max_cols

initialize_state_batched: Callable[[int], torch.Tensor] = c_ext.initialize_state_batched
get_action_mask_batched: Callable[[torch.Tensor], torch.Tensor] = c_ext.get_action_mask_batched
update_state_batched: Callable[[torch.Tensor, torch.Tensor], int] = c_ext.update_state_batched
