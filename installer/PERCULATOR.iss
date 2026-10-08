#define MyAppName "PERCULATOR"
#define MyAppVersion "0.2.1-alpha"
#define BuildRoot "..\\build\\PERCULATOR_artefacts\\Release"
[Setup]
AppId={{2FE20649-80D4-4C52-9264-E4043982C41A}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
DefaultDirName={autopf64}\\PERCULATOR
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\\dist
OutputBaseFilename=PERCULATOR_Setup_0.2.1_alpha_x64
Compression=lzma2
SolidCompression=yes
PrivilegesRequired=admin
UninstallDisplayName=PERCULATOR
[Types]
Name: "full"; Description: "VST3 and Standalone"
Name: "vst"; Description: "VST3 only"
[Components]
Name: "vst3"; Description: "VST3 plug-in"; Types: full vst; Flags: fixed
Name: "standalone"; Description: "Standalone with Cabinet IR loader"; Types: full
[Files]
Source: "{#BuildRoot}\\VST3\\PERCULATOR.vst3\\*"; DestDir: "{commoncf64}\\VST3\\PERCULATOR.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3
Source: "{#BuildRoot}\\Standalone\\PERCULATOR.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: standalone
[Icons]
Name: "{group}\\PERCULATOR"; Filename: "{app}\\PERCULATOR.exe"; Components: standalone
