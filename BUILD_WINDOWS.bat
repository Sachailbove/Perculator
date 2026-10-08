@echo off
setlocal
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 exit /b 1
cmake --build build --config Release --parallel
if errorlevel 1 exit /b 1
iscc installer\PERCULATOR.iss
if errorlevel 1 exit /b 1
echo Installer created in dist\PERCULATOR_Setup_0.2.1_alpha_x64.exe
