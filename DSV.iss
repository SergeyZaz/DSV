#define CDateY GetDateTimeString('yyyy', '#0', '#0')
#define CDateM GetDateTimeString('mm', '#0', '#0')
#define CDateD GetDateTimeString('dd', '#0', '#0')
#define CProgName "ДСВ"
#define CProg "DSV"


[Setup]
AppName={#CProgName}
AppVerName={#CProgName} версия: {#CDateY}.{#CDateM}.{#CDateD}
AppPublisher=Zaz, Inc.
DefaultDirName=c:\{#CProg}
DisableDirPage=yes
DefaultGroupName={#CProgName}
VersionInfoVersion={#CDateY}.{#CDateM}.{#CDateD}
AllowNoIcons=yes
LicenseFile=lic.txt
OutputDir=..\release
OutputBaseFilename={#CProg}_{#CDateY}{#CDateM}{#CDateD}
SetupIconFile=icon1.ico
Compression=lzma
SolidCompression=yes

[Languages]
Name: "rus"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\binr\*.*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs; 

[Icons]
Name: "{group}\Запуск программы"; WorkingDir: "{app}"; Filename: "{app}\{#CProg}.exe"
Name: "{group}\Удаление программы"; WorkingDir: "{app}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\{#CProgName}"; WorkingDir: "{app}"; Filename: "{app}\{#CProg}.exe"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#CProgName}"; WorkingDir: "{app}"; Filename: "{app}\{#CProg}.exe"; Tasks: quicklaunchicon

[InstallDelete]
Type: files; Name: "{app}\*.*"

[Run]
Filename: "{app}\vcredist_x64.exe"; Parameters: "/quiet"
Filename: "{app}\vcredist_x64_vc2013.exe"; Parameters: "/quiet"
Filename: "{app}\scripts\x86\create_db.bat"; WorkingDir: "{app}\scripts\x86";  Check: "not IsWin64" 
Filename: "{app}\scripts\create_db.bat"; WorkingDir: "{app}\scripts";  Check: IsWin64   

