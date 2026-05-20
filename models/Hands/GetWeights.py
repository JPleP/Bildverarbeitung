import torch
import numpy as np

checkpoint_path = 'Pose_Hand.pth'
state_dict = torch.load(checkpoint_path, map_location='cpu')
if 'state_dict' in state_dict:
    state_dict = state_dict['state_dict']

# List of all remaining structural keys we need to extract
gau_and_cls_keys = {
    "gau_gamma":       "head.gau.gamma",
    "gau_beta":        "head.gau.beta",
    "gau_o_weight":    "head.gau.o.weight",
    "gau_uv_weight":   "head.gau.uv.weight",
    "gau_ln_g":        "head.gau.ln.g",
    "gau_res_scale":   "head.gau.res_scale.scale",
    "cls_x_weight":    "head.cls_x.weight",
    "cls_y_weight":    "head.cls_y.weight"
}

for name, key in gau_and_cls_keys.items():
    if key in state_dict:
        arr = state_dict[key].detach().cpu().numpy().astype(np.float32)
        arr.tofile(f"{name}.bin")
        print(f"Saved {name}.bin with shape {arr.shape}")