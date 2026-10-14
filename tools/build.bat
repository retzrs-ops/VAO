@echo off
rem VAO をビルドする（Windows x64・RelWithDebInfo）。初回は OBS の部品を自動で取ってくるので時間がかかる。
setlocal
cd /d "%~dp0.."

cmake --preset windows-x64
if errorlevel 1 goto :fail

cmake --build --preset windows-x64
if errorlevel 1 goto :fail

echo.
echo ビルド成功: build_x64\rundir\RelWithDebInfo
exit /b 0

:fail
echo.
echo ビルド失敗。上のエラーの行を確認してください。
exit /b 1
