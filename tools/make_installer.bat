@echo off
rem インストーラー（exe）を作る。出力: dist\VAO-<版>-windows-x64-Setup.exe
rem 先に Inno Setup 6 を入れておくこと（無料）。
setlocal
cd /d "%~dp0.."

call tools\build.bat
if errorlevel 1 exit /b 1

set STAGE=%CD%\build_x64\stage
if exist "%STAGE%" rmdir /s /q "%STAGE%"
cmake --install build_x64 --config RelWithDebInfo --prefix "%STAGE%"
if errorlevel 1 goto :fail

set ISCC=
if exist "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" set ISCC=%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe
if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" set ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe
if exist "%ProgramFiles%\Inno Setup 6\ISCC.exe" set ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe
if "%ISCC%"=="" goto :noinno

for /f %%v in ('powershell -NoProfile -Command "(Get-Content buildspec.json -Raw | ConvertFrom-Json).version"') do set VER=%%v

"%ISCC%" /Qp "/DVersion=%VER%" "/DStageDir=%STAGE%" "/DRepoDir=%CD%" "/DOutputDir=%CD%\dist" installer\vao.iss
if errorlevel 1 goto :fail

echo.
echo インストーラーを作りました: dist\VAO-%VER%-windows-x64-Setup.exe
exit /b 0

:noinno
echo Inno Setup 6 が見つかりません。先にインストールしてください。
exit /b 1

:fail
echo.
echo インストーラーの作成に失敗しました。上のエラーの行を確認してください。
exit /b 1
