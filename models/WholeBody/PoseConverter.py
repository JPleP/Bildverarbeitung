import torch
import types
from mmpose.apis import init_model

config = 'Est_Body.py'
checkpoint = 'Est_Body.pth'

model = init_model(config, checkpoint, device='cpu')
model.eval()
model.forward_mode = 'tensor'



# 1. Setup your dummy tensor (B, C, H, W) -> Height=256, Width=192
dummy = torch.randn(1, 3, 256, 192)


# 2. Define a wrapper that matches what PNNX expects
class ModelWrapper(torch.nn.Module):
    def __init__(self, original_model):
        super().__init__()
        self.model = original_model
        # Put the model in evaluation mode
        self.model.eval()

    def forward(self, x):
        # OpenMMLab models often expect a list of tensors for data inputs
        # We pass an empty list or None for data_samples depending on the exact MM version
        # For MMpose 1.x / MMdet 3.x, passing data_samples=None in eval mode usually works:
        return self.model(x, data_samples=None, mode='tensor')

# 3. Instantiate the wrapper
wrapped_model = ModelWrapper(model)

import pnnx
# 4. Export the wrapped model instead
pnnx.export(wrapped_model, "Est_Body.pt", dummy)