@echo off
setlocal

set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build"
set "VCVARSALL=%VS_PATH%\vcvarsall.bat"

if exist "..\deps\install\OpenUSD" (
    echo Skipping execution because the directory ..\deps\install\OpenUSD exists.
    exit /b 0
)

if not exist "%VCVARSALL%" (
    echo Error: Visual Studio Developer Command Prompt is not installed or not found.
    exit /b 1
)

set "VC_ARCH=x64"

call "%VCVARSALL%" %VC_ARCH%
if %errorlevel% neq 0 (
    echo Error: Failed to set up Visual Studio Developer Command Prompt environment.
    exit /b 1
)

python ../deps/OpenUSD/build_scripts/build_usd.py ../deps/install/OpenUSD --no-python --build-monolithic --build-variant=debug --generator "Visual Studio 17 2022" --jobs 32 --build-args USD,"-DPXR_ENABLE_VULKAN_SUPPORT=TRUE"

if %errorlevel% neq 0 (
    echo Error: Command failed to execute.
    exit /b 1
)

echo Command executed successfully.
endlocal
exit /b 0