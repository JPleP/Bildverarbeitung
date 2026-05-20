from mmpose.apis import init_model
import torch
import pnnx

config = 'Pose_Hand.py'
checkpoint = 'Pose_Hand.pth'

base_model = init_model(config, checkpoint, device='cpu')
base_model.eval()

class RTMCCMlpExtractor(torch.nn.Module):
    def __init__(self, model):
        super().__init__()
        self.backbone = model.backbone
        self.final_layer = model.head.final_layer
        self.mlp = model.head.mlp

    def forward(self, x):
        feats = self.backbone(x)
        out = feats[-1] 
        
        out = self.final_layer(out)
        
        if len(out.shape) == 4:
            B, C, H, W = out.shape
            out = out.view(B, C, H * W)
                
        # Pass through the MLP safely
        out = self.mlp(out)
        return out

model = RTMCCMlpExtractor(base_model)
model.eval()

dummy = torch.randn(1, 3, 256, 256)
pnnx.export(model, "Pose_Hand_Mlp.pt", (dummy,))
print('NCNN Model (up to MLP) export complete!')