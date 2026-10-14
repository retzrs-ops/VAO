; VAO – Voice Auto Optimizer インストーラー（Inno Setup 6）
; 初心者が間違えないよう、選択肢を出さない:
;   ようこそ（「インストール」を押す）→ 完了（次の手順を表示）。準備完了の画面は出ない。
;   言語の選択は、Windows が日本語／英語なら出ない（ShowLanguageDialog=auto）。
; 入れ先は固定: C:\ProgramData\obs-studio\plugins\VAO
;   Windows 版の OBS が読むのは、この全ユーザー用の場所だけ（%APPDATA% 側は読まれない。2026-10 に実機で確認）。
;   ProgramData 直下には一般ユーザーでもフォルダを作れるので、管理者権限は要らない。
; ビルドは tools\make_installer.bat（手元）か、GitHub Actions の .github\scripts\Package-Windows.ps1。
;
; 渡す定義（ISCC /D...）:
;   Version   版（buildspec.json の version）
;   StageDir  cmake --install の出力先（中に VAO\bin\64bit と VAO\data がある）
;   RepoDir   リポジトリのルート
;   OutputDir 出力先フォルダ
;   OutputName 出力ファイル名（拡張子なし）。省略可

#ifndef Version
  #error Version を指定してください（/DVersion=0.2.0）
#endif
#ifndef StageDir
  #error StageDir を指定してください
#endif
#ifndef RepoDir
  #define RepoDir ".."
#endif
#ifndef OutputDir
  #define OutputDir "..\dist"
#endif
#ifndef OutputName
  #define OutputName "VAO-" + Version + "-windows-x64-Setup"
#endif

[Setup]
AppId={{7D3C1A52-9E4B-4F0A-8B6D-2A5E91C0F7B3}
AppName=VAO – Voice Auto Optimizer
AppVersion={#Version}
AppVerName=VAO {#Version}
AppPublisher=Retzn
AppPublisherURL=https://github.com/retzrs-ops/VAO
AppSupportURL=https://github.com/retzrs-ops/VAO
DefaultDirName={commonappdata}\obs-studio\plugins\VAO
UsePreviousAppDir=no
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableReadyPage=yes
; 準備完了の画面は、それより前に別の画面が1つも出ていないときだけ表示される仕様。
; ようこそ画面を出せば、準備完了の画面は出ない（Next ボタンの表示を「インストール」に変える: CurPageChanged）。
DisableWelcomePage=no
ShowLanguageDialog=auto
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
WizardStyle=modern
Compression=lzma2
SolidCompression=yes
OutputDir={#OutputDir}
OutputBaseFilename={#OutputName}
VersionInfoVersion={#Version}.0
VersionInfoDescription=VAO – Voice Auto Optimizer セットアップ
Uninstallable=yes
CloseApplications=no

[Languages]
Name: "ja"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "en"; MessagesFile: "compiler:Default.isl"

[Messages]
ja.WelcomeLabel2=OBS Studio の音声フィルタ「VAO Voice」を追加します。%n%n先に OBS Studio を終了してください。%n%n「インストール」を押すと、インストールが始まります。
ja.FinishedHeadingLabel=インストールが完了しました
ja.FinishedLabelNoIcons=次の手順で使えます。%n%n1. OBS Studio を起動する%n2. マイクの音声ソースを右クリック →「フィルタ」%n3.「音声フィルタ」の「＋」から「VAO Voice」を追加する%n4.「すべて自動設定（測定する）」を押し、画面の案内に従う（約26秒）%n%n※ フィルタ名の左にある「目」のアイコンがオンになっていることを確認してください。
en.WelcomeLabel2=This adds the audio filter "VAO Voice" to OBS Studio.%n%nPlease close OBS Studio first.%n%nClick Install to begin.
en.FinishedHeadingLabel=Installation complete
en.FinishedLabelNoIcons=How to use:%n%n1. Start OBS Studio%n2. Right-click your microphone source and choose Filters%n3. Click + under Audio Filters and add "VAO Voice"%n4. Press "Auto-set all (measures)" and follow the on-screen guide (about 26 seconds)%n%nMake sure the eye icon next to the filter name is turned on.

[CustomMessages]
ja.ObsRunning=OBS Studio が起動しています。%nOBS Studio を終了してから「再試行」を押してください。
ja.ObsNotFound=OBS Studio が見つかりませんでした。%n先に OBS Studio をインストールしてください。%n%nこのまま続けますか？
en.ObsRunning=OBS Studio is running.%nPlease close OBS Studio, then click Retry.
en.ObsNotFound=OBS Studio was not found.%nPlease install OBS Studio first.%n%nContinue anyway?

[InstallDelete]
; 配布前の試作インストーラーが入れていた場所（OBS には読まれない）
Type: filesandordirs; Name: "{userappdata}\obs-studio\plugins\VAO"

[Files]
Source: "{#StageDir}\VAO\bin\64bit\VAO.dll"; DestDir: "{app}\bin\64bit"; Flags: ignoreversion
Source: "{#StageDir}\VAO\data\*"; DestDir: "{app}\data"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#RepoDir}\Readme.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepoDir}\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepoDir}\licenses\*"; DestDir: "{app}\licenses"; Flags: ignoreversion

[Code]
// OBS が起動中か（起動中は DLL を置き換えられない）
function ObsIsRunning: Boolean;
var
  ResultCode: Integer;
begin
  Result := False;
  if Exec(ExpandConstant('{cmd}'), '/C tasklist /FI "IMAGENAME eq obs64.exe" | find /I "obs64.exe" > nul', '',
          SW_HIDE, ewWaitUntilTerminated, ResultCode) then
    Result := (ResultCode = 0);
end;

function ObsIsInstalled: Boolean;
begin
  Result := RegKeyExists(HKLM64, 'SOFTWARE\OBS Studio') or RegKeyExists(HKLM32, 'SOFTWARE\OBS Studio') or
            FileExists(ExpandConstant('{commonpf64}\obs-studio\bin\64bit\obs64.exe')) or
            FileExists(ExpandConstant('{commonpf32}\obs-studio\bin\64bit\obs64.exe'));
end;

function InitializeSetup: Boolean;
begin
  Result := True;

  while ObsIsRunning do
  begin
    if MsgBox(CustomMessage('ObsRunning'), mbInformation, MB_RETRYCANCEL) = IDCANCEL then
    begin
      Result := False;
      Exit;
    end;
  end;

  if not ObsIsInstalled then
  begin
    if MsgBox(CustomMessage('ObsNotFound'), mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDNO then
    begin
      Result := False;
      Exit;
    end;
  end;
end;

// ようこそ画面が、インストール前の最後の画面になる。「次へ」を「インストール」と表示して、迷わないようにする。
procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageID = wpWelcome then
    WizardForm.NextButton.Caption := SetupMessage(msgButtonInstall);
end;
