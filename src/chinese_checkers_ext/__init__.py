import torch
from . import _C as c_ext

# Expose C++ functions directly in the package namespace
initialize_state_batched = c_ext.initialize_state_batched
get_action_mask_batched = c_ext.get_action_mask_batched
update_state_batched = c_ext.update_state_batched
