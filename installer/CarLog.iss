#ifndef StageDir
  #define StageDir "..\build\installer-staging"
#endif
[Setup]
AppId={{E405BA9B-2470-41D4-8191-74E28DA80E26}
AppName=CarLog
AppVersion=1.0.0
AppPublisher=CarLog
DefaultDirName={localappdata}\Programs\CarLog
DefaultGroupName=CarLog
DisableProgramGroupPage=yes
DisableDirPage=no
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
OutputDir=..\build\installer
OutputBaseFilename=CarLogSetup
SetupIconFile=..\client\resources\carlog.ico
UninstallDisplayIcon={app}\carlog.ico
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Excludes: "carlog.ini"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#StageDir}\carlog.ini"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\CarLog"; Filename: "{app}\carlog.exe"; WorkingDir: "{app}"; IconFilename: "{app}\carlog.ico"
Name: "{userdesktop}\CarLog"; Filename: "{app}\carlog.exe"; WorkingDir: "{app}"; IconFilename: "{app}\carlog.ico"; Tasks: desktopicon

[Run]
Filename: "{app}\carlog.exe"; Description: "{cm:LaunchProgram,CarLog}"; Flags: nowait postinstall skipifsilent
