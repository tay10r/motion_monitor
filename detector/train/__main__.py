from autoencoder import Autoencoder

import torchvision.datasets as datasets
import torchvision.transforms as transforms
import torchvision.utils
import torch
import torch.nn as nn
import torch.utils
import torch.utils.data
import torch.optim as optim
import torch.onnx

def main():
    ae = Autoencoder()
    transform = transforms.Compose([
        transforms.Resize((256, 256)),
        transforms.ToTensor(),
    ])

    dataset = datasets.ImageFolder(root='data/', transform=transform)

    batch_size = 128

    loader = torch.utils.data.DataLoader(dataset=dataset, batch_size=batch_size, shuffle=True)

    criterion = nn.MSELoss()
    optimizer = optim.Adam(ae.parameters(), lr=0.001)
    num_epochs = 4
    for i in range(num_epochs):
        for data in loader:
            img, _ = data
            optimizer.zero_grad()
            output = ae(img)
            torchvision.utils.save_image(output, 'latest.png')
            loss = criterion(output, img)
            loss.backward()
            optimizer.step()
            print(f'Epoch [{i}/{num_epochs}]: {loss.item()}')


    torch.onnx.export(ae,
                      torch.randn(batch_size, 3, 256, 256),
                      'detector.onnx',
                      export_params=True,
                      opset_version=10,
                      do_constant_folding=True,
                      input_names=['input'],
                      output_names=['output'])

if __name__ == '__main__':
    main()
