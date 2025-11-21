# Building Binary Wheels for Mip-Splatting (Windows)

This document describes how to build binary wheels for the mip-splatting package on Windows, which includes CUDA extensions for differential Gaussian rasterization and simple k-NN operations.

> **Note:** This binary wheel build configuration is designed for **Windows only**. For Linux users, please use the original installation method from the README.

## Prerequisites

### Required Software
- **Windows 10/11** (64-bit)
- **Python 3.8 or higher** - [Download Python](https://www.python.org/downloads/)
- **Visual Studio 2019 or 2022** with C++ development tools
  - Download [Visual Studio Community](https://visualstudio.microsoft.com/)
  - During installation, select "Desktop development with C++"
- **CUDA Toolkit 11.8 or 12.1** - [Download CUDA](https://developer.nvidia.com/cuda-downloads)
  - CUDA 11.8 recommended for broader PyTorch compatibility
- **Git** (for cloning with submodules) - [Download Git](https://git-scm.com/download/win)

### Setting Up PyTorch

Install PyTorch with CUDA support matching your CUDA version:

**For CUDA 11.8:**
```cmd
pip install torch==2.2.2+cu118 torchvision==0.17.2+cu118 --extra-index-url https://download.pytorch.org/whl/cu118
```

**For CUDA 12.1:**
```cmd
pip install torch==2.2.2+cu121 torchvision==0.17.2+cu121 --extra-index-url https://download.pytorch.org/whl/cu121
```

Verify PyTorch installation:
```cmd
python -c "import torch; print(f'PyTorch: {torch.__version__}'); print(f'CUDA available: {torch.cuda.is_available()}')"
```

## Quick Build

### Using the Build Script

```cmd
build_wheel.bat
```

This script will:
1. Check for Python and CUDA installation
2. Install build dependencies
3. Clean previous builds
4. Build the binary wheel
5. Display the location of the built wheel

### Install the Built Wheel

```cmd
pip install dist\mip_splatting-0.1.0-*.whl
```

## Manual Build Instructions

### 1. Clone the Repository

```cmd
git clone --recursive https://github.com/autonomousvision/mip-splatting.git
cd mip-splatting
```

If you already cloned without `--recursive`, initialize submodules:
```cmd
git submodule update --init --recursive
```

### 2. Install Build Dependencies

```cmd
pip install wheel setuptools ninja build
```

### 3. Set Environment Variables

Ensure CUDA is properly configured:

```cmd
set CUDA_HOME=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.8
set PATH=%CUDA_HOME%\bin;%PATH%
```

(Adjust the path if you installed a different CUDA version)

### 4. Build the Wheel

```cmd
python -m build --wheel
```

The wheel will be created in the `dist\` directory.

### 5. Install the Wheel

```cmd
pip install dist\*.whl
```

## Testing the Installation

After installation, verify that the CUDA extensions are working:

```python
import diff_gaussian_rasterization
import simple_knn
print("Mip-Splatting extensions loaded successfully!")
```

Or from command line:
```cmd
python -c "import diff_gaussian_rasterization; import simple_knn; print('Success!')"
```

## Environment Variables

### CUDA Architecture

By default, the build compiles for multiple CUDA architectures (6.0 through 9.0). To compile for specific architectures only (faster build):

```cmd
set TORCH_CUDA_ARCH_LIST=7.5;8.0;8.6
python -m build --wheel
```

### Controlling Parallel Jobs

By default, the build uses max 2 parallel jobs to prevent resource exhaustion. To change this:

```cmd
set MAX_JOBS=4
python -m build --wheel
```

## Supported CUDA Architectures

The binary wheel supports the following NVIDIA GPU architectures by default:

| Architecture | Compute Capability | Example GPUs |
|--------------|-------------------|--------------|
| Pascal       | 6.0, 6.1         | GTX 1050/1060/1070/1080, GTX 1650 |
| Volta        | 7.0              | Titan V, Tesla V100 |
| Turing       | 7.5              | RTX 2060/2070/2080, GTX 1660, T4 |
| Ampere       | 8.0, 8.6         | A100, RTX 3060/3070/3080/3090 |
| Ada Lovelace | 8.9              | RTX 4060/4070/4080/4090 |
| Hopper       | 9.0              | H100 |

## Troubleshooting

### Error: "nvcc: command not found" or "nvcc is not recognized"

**Solution:** CUDA is not in your PATH. Add it:
```cmd
set PATH=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.8\bin;%PATH%
```

Or permanently add it via System Environment Variables.

### Error: "Microsoft Visual C++ 14.0 or greater is required"

**Solution:** Install Visual Studio with C++ development tools:
1. Download [Visual Studio Community](https://visualstudio.microsoft.com/)
2. Run installer and select "Desktop development with C++"
3. Restart your command prompt

### Warning: "member ... was declared deprecated"

This is normal and safe to ignore. The build includes `/wd4624` flag to suppress this warning.

### Error: "CUDA out of memory during build"

**Solution:** Reduce parallel jobs:
```cmd
set MAX_JOBS=1
python -m build --wheel
```

### Error: "Unsupported gpu architecture 'compute_90'"

Your CUDA version might be too old for Hopper (H100) GPUs. Edit `setup.py` and remove newer architectures:

```python
# Around line 30-36, remove 8.9 and 9.0:
cuda_archs = [
    '6.0', '6.1',  # Pascal
    '7.0', '7.5',  # Volta, Turing
    '8.0', '8.6',  # Ampere
]
```

### Import Error: "cannot import name '_C'"

The CUDA extension failed to build. Verify:

1. **CUDA toolkit is installed:**
   ```cmd
   nvcc --version
   ```

2. **PyTorch has CUDA support:**
   ```cmd
   python -c "import torch; print(torch.cuda.is_available())"
   ```
   Should print `True`

3. **Check build logs** for compilation errors in the output

### Build is very slow

This is normal. Building CUDA extensions takes 5-15 minutes depending on your system. The build compiles for multiple GPU architectures which takes time.

## Automated Builds (GitHub Actions)

The repository includes a GitHub Actions workflow (`.github/workflows/build_binary_wheel.yml`) that automatically builds wheels for:
- **Windows** (windows-latest)
- **Python versions:** 3.8, 3.9, 3.10, 3.11
- **CUDA versions:** 11.8, 12.1

Wheels are uploaded as artifacts and retained for 30 days. You can download pre-built wheels from the Actions tab after a successful build.

## Distribution

### Sharing the Wheel

The `.whl` file in the `dist\` directory is a fully self-contained binary wheel. You can share it with others who have:
- The same Python version (e.g., Python 3.10)
- The same CUDA version (or compatible)
- Windows 64-bit

Users can install with:
```cmd
pip install mip_splatting-0.1.0-cp310-cp310-win_amd64.whl
```

### PyPI Upload (Maintainers Only)

```cmd
pip install twine
twine upload dist\*.whl
```

## Development Installation (Alternative)

If you're developing and frequently changing the CUDA code, you may prefer the original editable installation method:

```cmd
pip install -e submodules/diff-gaussian-rasterization
pip install -e submodules/simple-knn
```

This allows you to make changes and rebuild quickly with:
```cmd
cd submodules/diff-gaussian-rasterization
python setup.py build_ext --inplace
```

## Comparison with Original Installation

**Original method (from README):**
```cmd
pip install submodules/diff-gaussian-rasterization
pip install submodules/simple-knn/
```
- ❌ Requires compilation on each machine
- ❌ Requires CUDA toolkit and Visual Studio on all machines
- ❌ Takes 10-15 minutes to install

**Binary wheel method:**
```cmd
pip install dist\mip_splatting-0.1.0-*.whl
```
- ✅ Pre-compiled binary (installs in seconds)
- ✅ No CUDA toolkit needed on deployment machines
- ✅ No Visual Studio needed on deployment machines
- ✅ Single file to distribute
- ✅ Reproducible builds

## System Requirements

**Minimum:**
- Windows 10 64-bit
- NVIDIA GPU with CUDA capability 6.0+ (GTX 10-series or newer)
- 8 GB RAM
- 5 GB free disk space (for build process)

**Recommended:**
- Windows 11 64-bit
- NVIDIA RTX GPU (20-series or newer)
- 16 GB RAM
- SSD with 10 GB free space

## License

The built wheels inherit the license from the original mip-splatting project: **free for non-commercial, research and evaluation use** under the terms of LICENSE.md.

## Getting Help

If you encounter issues:

1. Check this troubleshooting section
2. Verify all prerequisites are installed correctly
3. Check the [GitHub Issues](https://github.com/autonomousvision/mip-splatting/issues)
4. For CUDA/PyTorch issues, consult the [PyTorch installation guide](https://pytorch.org/get-started/locally/)

---

**Note for Linux/Mac Users:** This wheel build is Windows-only. For other platforms, use the original installation method:
```bash
pip install submodules/diff-gaussian-rasterization
pip install submodules/simple-knn/
```
