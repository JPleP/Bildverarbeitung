from mmdet.apis import init_detector
import torch

config = 'Det_Hand.py'
checkpoint = 'Det_Hand.pth'

model = init_detector(config, checkpoint, device='cpu')
model.eval()

dummy = torch.randn(1, 3, 320, 320)

torch.onnx.export(
    model,
    dummy,
    'Det_Hand.onnx',
    opset_version=12,
    do_constant_folding=True,
    input_names=['images'],
    output_names=['output'],
    dynamic_axes=None
)

print('ONNX export done')