from setuptools import setup, find_packages
from torch.utils.cpp_extension import CUDAExtension, BuildExtension
import os
import sys

root_dir = os.path.dirname(os.path.abspath(__file__))

# This binary wheel build is designed for Windows only
if os.name != 'nt':
    print("=" * 70)
    print("ERROR: This binary wheel build is configured for Windows only.")
    print("For Linux installation, please use the original method:")
    print("  pip install submodules/diff-gaussian-rasterization")
    print("  pip install submodules/simple-knn/")
    print("=" * 70)
    sys.exit(1)

# Windows-specific compiler flags for MSVC + CUDA compatibility
cxx_flags = [
    '/D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH',
    '/D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS',
    "/wd4624"  # destructor was implicitly defined as deleted
]

nvcc_flags = [
    '-allow-unsupported-compiler',
    '-D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH',
    '--expt-relaxed-constexpr',
    '--expt-extended-lambda',
]

# Path to GLM library in diff-gaussian-rasterization
glm_include = os.path.join(root_dir, "submodules/diff-gaussian-rasterization/third_party/glm/")
nvcc_flags.append("-I" + glm_include)

# CUDA architecture list for broad GPU compatibility
# Covers GPUs from Pascal (GTX 10xx) to Ada/Hopper (RTX 40xx, H100)
cuda_archs = [
    '6.0', '6.1',  # Pascal (GTX 10xx, P100)
    '7.0', '7.5',  # Volta (V100), Turing (RTX 20xx)
    '8.0', '8.6',  # Ampere (A100, RTX 30xx)
    '8.9',         # Ada (RTX 40xx)
    '9.0'          # Hopper (H100)
]

setup(
    name="mip_splatting",
    version="0.1.0",
    author="Zehao Yu",
    author_email="",
    description="Mip-Splatting: Alias-free 3D Gaussian Splatting",
    long_description=open(os.path.join(root_dir, "README.md"), encoding="utf-8").read(),
    long_description_content_type="text/markdown",
    url="https://github.com/autonomousvision/mip-splatting",
    packages=find_packages() + [
        'diff_gaussian_rasterization',
        'simple_knn'
    ],
    package_dir={
        'diff_gaussian_rasterization': 'submodules/diff-gaussian-rasterization/diff_gaussian_rasterization',
        'simple_knn': 'submodules/simple-knn/simple_knn'
    },
    ext_modules=[
        # Differential Gaussian Rasterization extension
        CUDAExtension(
            name="diff_gaussian_rasterization._C",
            sources=[
                "submodules/diff-gaussian-rasterization/cuda_rasterizer/rasterizer_impl.cu",
                "submodules/diff-gaussian-rasterization/cuda_rasterizer/forward.cu",
                "submodules/diff-gaussian-rasterization/cuda_rasterizer/backward.cu",
                "submodules/diff-gaussian-rasterization/rasterize_points.cu",
                "submodules/diff-gaussian-rasterization/ext.cpp"
            ],
            extra_compile_args={
                'cxx': cxx_flags,
                'nvcc': nvcc_flags
            }
        ),
        # Simple KNN extension
        CUDAExtension(
            name="simple_knn._C",
            sources=[
                "submodules/simple-knn/spatial.cu",
                "submodules/simple-knn/simple_knn.cu",
                "submodules/simple-knn/ext.cpp"
            ],
            extra_compile_args={
                'cxx': cxx_flags,
                'nvcc': nvcc_flags if nvcc_flags and nvcc_flags[0] != '-I' + glm_include else []
            }
        )
    ],
    cmdclass={
        'build_ext': BuildExtension.with_options(
            use_ninja=True,
            max_jobs=2  # Limit parallel jobs on Windows to prevent resource exhaustion
        )
    },
    install_requires=[
        "torch>=1.12.1",
        "open3d",
        "plyfile",
        "ninja",
        "GPUtil",
        "opencv-python",
        "lpips"
    ],
    python_requires=">=3.8",
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Science/Research",
        "Topic :: Scientific/Engineering :: Artificial Intelligence",
        "License :: Free for non-commercial use",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
    ],
)
