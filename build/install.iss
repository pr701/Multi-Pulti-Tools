#define app "Конструктор мультфильмов"
#define bin "multic.exe"
#define publisher "Basi"
#define ver "1.0"

[Setup]
ArchitecturesAllowed=x86 x64
DisableWelcomePage=No
WizardImageFile={style}\Wizard.bmp
WizardSmallImageFile={style}\Small.bmp
WizardImageBackColor=clWhite
AppName={#app}
AppVersion={#ver}
AppCopyright=Basi
AllowCancelDuringInstall=True
DefaultDirName={pf32}\{#app}
DisableProgramGroupPage=auto
AppPublisher={#publisher}
AppComments=rebuild by painter
UninstallDisplayName={#app}
VersionInfoVersion={#ver}
VersionInfoCompany={#publisher}
VersionInfoTextVersion={#ver}
VersionInfoProductVersion={#ver}
SolidCompression=True
ShowComponentSizes=False
Compression=lzma2/ultra64
InternalCompressLevel=ultra
CompressionThreads=2
MinVersion=0,5.01
OutputBaseFilename=multi-setup
RestartIfNeededByRun=False
VersionInfoCopyright={#publisher}
AllowUNCPath=False
ShowLanguageDialog=no
LanguageDetectionMethod=none
AllowNoIcons=True
OutputDir=output
DefaultGroupName={#app}
UpdateUninstallLogAppName=False

[Files]
; Style
Source: "{style}\Welcome.bmp"; DestDir: "{tmp}"; DestName: "Welcome.bmp"; Flags: deleteafterinstall ignoreversion noencryption
; Files
Source: "{game}\*"; DestDir: "{app}"; Flags: recursesubdirs uninsrestartdelete
; Binary
Source: "{bin}\*"; DestDir: "{app}"; Flags: recursesubdirs uninsrestartdelete

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "fullscreenmode"; Description: "{cm:Fullscreen}"; GroupDescription: "{cm:Extra}"

[Icons]
Name: "{group}\{#app}"; Filename: "{app}\{#bin}"; WorkingDir: "{app}"; Comment: "Запустить {#app}"
Name: "{group}\Удалить {#app}"; Filename: "{uninstallexe}"; Comment: "Удаление игры с компьютера"
Name: "{group}\Руководство"; Filename: "{app}\read.me"; WorkingDir: "{app}"; Flags: foldershortcut; Comment: "Открыть руководство"
Name: "{commondesktop}\{#app}"; Filename: "{app}\{#bin}"; WorkingDir: "{app}"; Comment: "Запустить конструктор"; Tasks: desktopicon

[CustomMessages]
Extra=Дополнительно
Fullscreen=Запускать конструктор в полноэкранном режиме

[Run]
Filename: "{app}\{#bin}"; WorkingDir: "{app}"; Flags: postinstall unchecked; Description: "Запустить {#app}"

[INI]
Filename: "{app}\Setup.ini"; Section: "Param"; Key: "Fullscreen"; String: "1"; Tasks: fullscreenmode

[Code]
procedure InitializeWizard();
var
  BitmapImage: TBitmapImage;
begin
  ExtractTemporaryFile('Welcome.bmp');
  // Welcome page }
  // Hide the labels }
  WizardForm.WelcomeLabel1.Visible := False;
  WizardForm.WelcomeLabel2.Visible := False;
  // Stretch image over whole page }
  WizardForm.WizardBitmapImage.Width := WizardForm.WizardBitmapImage.Parent.Width;
  WizardForm.WizardBitmapImage.Bitmap.LoadFromFile(ExpandConstant('{tmp}\Welcome.bmp')); 

  // Finished page
  // Hide the labels
  // WizardForm.FinishedLabel.Visible := False;
  // WizardForm.FinishedHeadingLabel.Visible := False;
  // Stretch image over whole page }
  // WizardForm.WizardBitmapImage2.Width := WizardForm.WizardBitmapImage2.Parent.Width; 
end;
