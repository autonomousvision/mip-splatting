# Building Binary Wheels for Mip-Splatting

This document describes how to build binary wheels for the mip-splatting package, which includes CUDA extensions for differential Gaussian rasterization and simple k-NN operations.

## Prerequisites

### All Platforms
- Python 3.8 or higher
- PyTorch (compatible with your CUDA version)
- CUDA Toolkit 11.3, 11.8, or 12.1
- Git (for cloning with submodules)

### Linux
```bash
# Install CUDA Toolkit (example for CUDA 11.8)
wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2004/x86_64/cuda-keyring_1.0-1_all.deb
sudo dpkg -i cuda-keyring_1.0-1_all.deb
sudo apt-get update
sudo apt-get install cuda-11-8

# Install PyTorch
pip install torch==2.0.1+cu118 torchvision==0.15.2+cu118 --extra-index-url https://download.pytorch.org/whl/cu118
```

### Windows
1. Install [Visual Studio 2019 or 2022](https://visualstudio.microsoft.com/) with C++ development tools
2. Install [CUDA Toolkit](https://developer.nvidia.com/cuda-downloads) (11.8 or 12.1 recommended)
3. Install PyTorch:
   ```cmd
   pip install torch==2.2.2+cu118 torchvision==0.17.2+cu118 --extra-index-url https://download.pytorch.org/whl/cu118
   ```

## Quick Build

### Using the Build Script

**Linux/macOS:**
```bash
./build_wheel.sh
```

**Windows:**
```cmd
build_wheel.bat
```

### Manual Build

1. **Clone the repository with submodules:**
   ```bash
   git clone --recursive https://github.com/autonomousvision/mip-splatting.git
   cd mip-splatting
   ```

2. **Install build dependencies:**
   ```bash
   pip install wheel setuptools ninja build
   ```

3. **Build the wheel:**
   ```bash
   python -m build --wheel
   ```

4. **Install the built wheel:**
   ```bash
   pip install dist/*.whl
   ```

## Testing the Installation

After installation, verify that the CUDA extensions are working:

```python
import diff_gaussian_rasterization
import simple_knn
print("Mip-Splatting extensions loaded successfully!")
```

## Environment Variables

### CUDA Architecture
By default, the build compiles for multiple CUDA architectures (6.0 through 9.0). To compile for specific architectures only:

```bash
export TORCH_CUDA_ARCH_LIST="7.5;8.0;8.6"  # Example: RTX 20xx, A100, RTX 30xx
python -m build --wheel
```

### CUDA Home
If CUDA is not in the default location:

**Linux/macOS:**
```bash
export CUDA_HOME=/usr/local/cuda-11.8
export PATH=$CUDA_HOME/bin:$PATH
export LD_LIBRARY_PATH=$CUDA_HOME/lib64:$LD_LIBRARY_PATH
```

**Windows:**
```cmd
set CUDA_HOME=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.8
set PATH=%CUDA_HOME%\bin;%PATH%
```

## Supported CUDA Architectures

The binary wheel supports the following GPU architectures by default:

| Architecture | Compute Capability | Example GPUs |
|--------------|-------------------|--------------|
| Pascal       | 6.0, 6.1         | GTX 10xx, P100 |
| Volta        | 7.0              | V100 |
| Turing       | 7.5              | RTX 20xx, T4 |
| Ampere       | 8.0, 8.6         | A100, RTX 30xx |
| Ada          | 8.9              | RTX 40xx |
| Hopper       | 9.0              | H100 |

## Troubleshooting

### CUDA out of memory during build
Reduce the number of parallel jobs:
```bash
MAX_JOBS=2 python -m build --wheel
```

### Windows: "error: member ... was declared deprecated"
This is usually safe to ignore. The build includes `/wd4624` to suppress this warning.

### Linux: "nvcc fatal : Unsupported gpu architecture 'compute_90'"
Your CUDA version might be too old. Either upgrade CUDA or modify `setup.py` to remove unsupported architectures:
```python
cuda_archs = ['7.0', '7.5', '8.0', '8.6']  # Remove 8.9 and 9.0
```

### Import error: "cannot import name '_C'"
The CUDA extension failed to build. Check that:
1. CUDA toolkit is properly installed
2. PyTorch is installed with CUDA support: `python -c "import torch; print(torch.cuda.is_available())"`
3. NVCC is in PATH: `nvcc --version`

## Automated Builds (GitHub Actions)

The repository includes a GitHub Actions workflow (`.github/workflows/build_binary_wheel.yml`) that automatically builds wheels for:
- Ubuntu 20.04 and Windows
- Python 3.8, 3.9, 3.10, 3.11
- CUDA 11.3, 11.8, 12.1

Wheels are uploaded as artifacts and retained for 30 days.

## Distribution

### PyPI Upload (Maintainers)
```bash
pip install twine
twine upload dist/*.whl
```

### Local Distribution
Share the `.whl` file from the `dist/` directory. Users can install with:
```bash
pip install mip_splatting-0.1.0-cp310-cp310-linux_x86_64.whl
```

## Development Installation

For development, you can still install the submodules individually:
```bash
pip install -e submodules/diff-gaussian-rasterization
pip install -e submodules/simple-knn
```

This allows you to make changes to the CUDA code and rebuild quickly without creating a full wheel.

## Comparison with Original Installation

**Original method (from README):**
```bash
pip install submodules/diff-gaussian-rasterization
pip install submodules/simple-knn/
```

**New binary wheel method:**
```bash
pip install dist/mip_splatting-0.1.0-*.whl
```

The binary wheel approach:
- ✅ Faster installation (no compilation needed)
- ✅ More reliable (pre-compiled, tested binaries)
- ✅ Better for deployment (single file to distribute)
- ✅ Cross-platform compatible (with architecture matching)

## License

The built wheels inherit the license from the original mip-splatting project: free for non-commercial, research and evaluation use under the terms of LICENSE.md.
