@echo off
setlocal
cd /d "%~dp0.."

cmake --build --preset x64-debug
if %errorlevel% neq 0 exit /b %errorlevel%

cmake --build --preset x64-debug --target test
exit /b %errorlevel%
