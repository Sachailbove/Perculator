# PERCULATOR 0.1.0-alpha

Circuit-inspired dual-polarity distortion plug-in with an optional user-supplied cabinet IR loader.

## Included
- VST3 and Standalone targets for Windows x64
- Harmonics, Balance, Input, Bias, Mix and Output
- Original/Studio mode
- User IR loading by button or drag-and-drop: WAV, AIFF and FLAC
- IR on/off, mix, level and 180-degree phase
- Three persistent panel finishes: Dictatorship, Surf Party and Fetish Red
- Inno Setup installer recipe
- GitHub Actions Windows build workflow

## Local Windows build
Install Visual Studio 2022 with Desktop development with C++, Git, CMake 3.22+ and Inno Setup.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
iscc installer\PERCULATOR.iss
```

Final installer: `dist\PERCULATOR_Setup_0.1.0_alpha_x64.exe`.

## Alpha notes
The non-linear stage is circuit-inspired and not yet calibrated against a measured physical unit. No IR files are included. The preset stores the IR path and reloads the file when available.
