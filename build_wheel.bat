@echo off
REM Build script for mip-splatting binary wheel (Windows)
REM Usage: build_wheel.bat

echo ========================================
echo Building mip-splatting binary wheel
echo ========================================

REM Check if Python is available
python --version >nul 2>&1
if errorlevel 1 (
    echo Error: Python not found. Please install Python 3.8 or higher.
    exit /b 1
)

REM Check if CUDA is available
nvcc --version >nul 2>&1
if errorlevel 1 (
    echo Warning: nvcc not found. CUDA might not be properly installed.
    echo Please ensure CUDA toolkit is installed and CUDA_HOME is set.
)

REM Print versions
echo.
echo Python version:
python --version
echo.
echo CUDA version (if available):
nvcc --version 2>nul || echo nvcc not found
echo.

REM Install build dependencies
echo Installing build dependencies...
pip install -q --upgrade pip wheel setuptools ninja build

REM Clean previous builds
echo Cleaning previous builds...
if exist build rmdir /s /q build
if exist dist rmdir /s /q dist
for /d %%i in (*.egg-info) do rmdir /s /q "%%i"
for /d %%i in (submodules\*\build) do rmdir /s /q "%%i"
for /d %%i in (submodules\*\*.egg-info) do rmdir /s /q "%%i"

REM Build the wheel
echo Building wheel...
python -m build --wheel

REM Display the built wheel
echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo Built wheel:
dir /b dist\*.whl

echo.
echo To install the wheel, run:
echo   pip install dist\*.whl
echo.
echo To test the installation:
echo   python -c "import diff_gaussian_rasterization; import simple_knn; print('Success!')"
