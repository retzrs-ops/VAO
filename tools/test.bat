@echo off
rem ビルドして、音声処理のテスト（OBS なしで動く）を ctest で実行する。
setlocal
cd /d "%~dp0.."

call tools\build.bat
if errorlevel 1 exit /b 1

ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure
if errorlevel 1 goto :fail

echo.
echo テスト成功: FAIL 0 件
exit /b 0

:fail
echo.
echo テスト失敗。上の FAIL の行を確認してください。
exit /b 1
