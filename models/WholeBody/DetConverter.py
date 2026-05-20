from mmdet.apis import init_detector
import torch
import pnnx

config = 'Det_Body.py'
checkpoint = 'Det_Body.pth'

model = init_detector(config, checkpoint, device='cpu')
model.eval()

dummy = torch.randn(1, 3, 320, 320)

pnnx.export(model, "Det_Body.pt", (dummy,))
print('NCNN Model (up to MLP) export complete!')

print('ONNX export done')