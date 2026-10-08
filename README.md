# PERCULATOR 0.2.1-alpha

Windows x64 VST3 and Standalone circuit-inspired distortion with user cabinet IR loader.

## Changes in 0.2.0
- New resizable pedal-style GUI matching the approved Surf Party preview
- Panel finishes: Dictatorship, Surf Party and Fetish Red
- Semicircular multicolour LED meters around Input and Output
- Original/Studio switch removed: Studio behaviour is always active
- Mono/Stereo mode switch removed: channel layout follows the host automatically
- Three-position Circuit control: NPN OD, D310 Diodes and Albino
- User IR loading by button or drag-and-drop, with IR Mix, Level and phase
- No factory IR files included

## GitHub build
Place all files at the root of your repository. The workflow is `.github/workflows/windows-build.yml`.
Open Actions, choose `Windows x64 build`, then `Run workflow`. The artifact is named `PERCULATOR-Windows-x64-v0.2.1-alpha`.

## Important alpha note
The circuit models are circuit-inspired approximations and have not yet been calibrated against measured hardware. Albino is now based on the supplied Sardonic Albinator / Alex Frias topology: 2N404A PNP germanium (hFE 45), 2N3565 NPN silicon (hFE 265), 1N695 diode clipping, C3 2.2 uF and C7 1.5 nF. It remains a stable circuit-inspired real-time model rather than a transistor-level SPICE solver, and can be calibrated further with audio measurements.
