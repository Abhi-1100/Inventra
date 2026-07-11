@echo off
echo ===================================================
echo Kirana Terminal — Clean Rebuild Utility
echo ===================================================

:: 1. Locate Visual Studio vcvarsall.bat
set VCVARS_PATH="C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat"

if not exist %VCVARS_PATH% (
    echo [ERROR] Visual Studio Community 2025/2026 was not found at %VCVARS_PATH%
    echo Please edit this script with your correct Visual Studio path.
    pause
    exit /b 1
)

:: 2. Initialize developer environment
echo [1/3] Initializing Developer Command Prompt (x64)...
call %VCVARS_PATH% amd64
if %errorlevel% neq 0 (
    echo [ERROR] Failed to initialize compiler environment.
    pause
    exit /b 1
)

:: 3. Clean build directory
if exist build (
    echo [2/3] Cleaning build folder...
    rmdir /s /q build
)

:: 4. Configure with CMake using NMake Makefiles
echo [3/3] Configuring project with CMake...
cmake -B build -G "NMake Makefiles" -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/msvc2022_64"
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed.
    pause
    exit /b 1
)

:: 5. Compile and deploy
echo [4/4] Compiling project...
cd build
nmake
if %errorlevel% neq 0 (
    echo [ERROR] Compilation failed.
    cd ..
    pause
    exit /b 1
)

echo ===================================================
echo Build Succeeded! Executable ready at:
echo D:\confres\build\KiranaTerminal.exe
echo ===================================================
cd ..
pause
