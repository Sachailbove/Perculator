@echo off
setlocal
where cmake >nul 2>nul || (echo ERRORE: CMake non trovato.& exit /b 1)
where iscc >nul 2>nul || (echo ERRORE: Inno Setup non trovato nel PATH.& exit /b 1)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 || exit /b 1
cmake --build build --config Release --parallel || exit /b 1
if not exist dist mkdir dist
iscc installer\PERCULATOR.iss || exit /b 1
echo.
echo Build completata: dist\PERCULATOR_Setup_0.1.0_alpha_x64.exe
endlocal
