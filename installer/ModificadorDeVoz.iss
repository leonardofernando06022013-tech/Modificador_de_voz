#define AppName "Modificador de Voz"
#define AppVersion "1.0.0"
[Setup]
AppId={{E0F55811-CB88-4F73-9AA8-4639FCA803A1}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=Modificador de Voz
DefaultDirName={autopf}\Modificador de Voz
DefaultGroupName=Modificador de Voz
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir=..\dist\installer
OutputBaseFilename=Modificador-de-Voz-Setup-x64
SetupIconFile=..\resources\icons\ModificadorDeVoz.ico
UninstallDisplayIcon={app}\ModificadorDeVoz.exe
Compression=lzma2
SolidCompression=yes
CloseApplications=yes
[Files]
Source: "..\build\ModificadorDeVoz_artefacts\Release\ModificadorDeVoz.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\resources\*"; DestDir: "{app}\Resources"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
[Tasks]
Name: desktopicon; Description: "Criar um atalho na Área de Trabalho"; GroupDescription: "Atalhos adicionais:"; Flags: checkedonce
[Icons]
Name: "{group}\Modificador de Voz"; Filename: "{app}\ModificadorDeVoz.exe"; WorkingDir: "{app}"; IconFilename: "{app}\ModificadorDeVoz.exe"; Comment: "Modificador de voz em tempo real"
Name: "{autodesktop}\Modificador de Voz"; Filename: "{app}\ModificadorDeVoz.exe"; WorkingDir: "{app}"; IconFilename: "{app}\ModificadorDeVoz.exe"; Comment: "Modificador de voz em tempo real"; Tasks: desktopicon
[Run]
Filename: "{app}\ModificadorDeVoz.exe"; Description: "Executar Modificador de Voz"; Flags: nowait postinstall skipifsilent
