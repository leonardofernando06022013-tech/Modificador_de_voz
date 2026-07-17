#define AppName "BlackVoice"
#define AppVersion "1.0.0"
[Setup]
AppId={{E0F55811-CB88-4F73-9AA8-4639FCA803A1}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=BlackVoice
DefaultDirName={autopf}\BlackVoice
DefaultGroupName=BlackVoice
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir=..\dist\installer
OutputBaseFilename=BlackVoice-Setup-x64
SetupIconFile=..\resources\icons\BlackVoice.ico
UninstallDisplayIcon={app}\BlackVoice.exe
Compression=lzma2
SolidCompression=yes
CloseApplications=yes
[Files]
Source: "..\build\BlackVoice_artefacts\Release\BlackVoice.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\resources\*"; DestDir: "{app}\Resources"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
[Tasks]
Name: desktopicon; Description: "Criar um atalho na Área de Trabalho"; GroupDescription: "Atalhos adicionais:"; Flags: checkedonce
[Icons]
Name: "{group}\BlackVoice"; Filename: "{app}\BlackVoice.exe"; WorkingDir: "{app}"; IconFilename: "{app}\BlackVoice.exe"; Comment: "Modificador de voz em tempo real"
Name: "{autodesktop}\BlackVoice"; Filename: "{app}\BlackVoice.exe"; WorkingDir: "{app}"; IconFilename: "{app}\BlackVoice.exe"; Comment: "Modificador de voz em tempo real"; Tasks: desktopicon
[Run]
Filename: "{app}\BlackVoice.exe"; Description: "Executar BlackVoice"; Flags: nowait postinstall skipifsilent
