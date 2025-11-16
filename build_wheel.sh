#!/bin/bash
# Build script for mip-splatting binary wheel
# Usage: ./build_wheel.sh

set -e

echo "========================================"
echo "Building mip-splatting binary wheel"
echo "========================================"

# Check if Python is available
if ! command -v python &> /dev/null; then
    echo "Error: Python not found. Please install Python 3.8 or higher."
    exit 1
fi

# Check if CUDA is available
if ! command -v nvcc &> /dev/null; then
    echo "Warning: nvcc not found. CUDA might not be properly installed."
    echo "Please ensure CUDA toolkit is installed and CUDA_HOME is set."
fi

# Print versions
echo ""
echo "Python version:"
python --version
echo ""
echo "CUDA version (if available):"
nvcc --version 2>/dev/null || echo "nvcc not found"
echo ""

# Install build dependencies
echo "Installing build dependencies..."
pip install -q --upgrade pip wheel setuptools ninja build

# Clean previous builds
echo "Cleaning previous builds..."
rm -rf build dist *.egg-info
rm -rf submodules/*/build submodules/*/*.egg-info

# Build the wheel
echo "Building wheel..."
python -m build --wheel

# Display the built wheel
echo ""
echo "========================================"
echo "Build completed successfully!"
echo "========================================"
echo ""
echo "Built wheel:"
ls -lh dist/*.whl

echo ""
echo "To install the wheel, run:"
echo "  pip install dist/*.whl"
echo ""
echo "To test the installation:"
echo "  python -c 'import diff_gaussian_rasterization; import simple_knn; print(\"Success!\")'"
