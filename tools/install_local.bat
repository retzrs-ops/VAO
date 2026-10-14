@echo off
rem ビルド済みの VAO を手元の OBS に入れる。OBS は先に終了しておくこと。
rem 入れ先: C:\ProgramData\obs-studio\plugins\VAO
setlocal
cd /d "%~dp0.."

set SRC=build_x64\rundir\RelWithDebInfo
set DEST=C:\ProgramData\obs-studio\plugins\VAO

if not exist "%SRC%\VAO.dll" goto :nobuild

tasklist /fi "imagename eq obs64.exe" | find /i "obs64.exe" > nul
if not errorlevel 1 goto :obsrunning
ver > nul

if not exist "%DEST%\bin\64bit" mkdir "%DEST%\bin\64bit"
if errorlevel 1 goto :fail
if not exist "%DEST%\data" mkdir "%DEST%\data"
if errorlevel 1 goto :fail

copy /y "%SRC%\VAO.dll" "%DEST%\bin\64bit\" > nul
if errorlevel 1 goto :fail
if exist "%SRC%\VAO.pdb" copy /y "%SRC%\VAO.pdb" "%DEST%\bin\64bit\" > nul
xcopy /e /i /y "%SRC%\VAO\*" "%DEST%\data\" > nul
if errorlevel 1 goto :fail

echo.
echo 導入しました: %DEST%
echo OBS を起動し、音声ソースのフィルタ追加で「VAO Voice」を探してください。
pause
exit /b 0

:nobuild
echo ビルド済みのファイルがありません。先に tools\build.bat を実行してください。
pause
exit /b 1

:obsrunning
echo OBS が起動中です。OBS を終了してからもう一度実行してください。
pause
exit /b 1

:fail
echo コピーに失敗しました。管理者として実行する必要があるかもしれません。
pause
exit /b 1
