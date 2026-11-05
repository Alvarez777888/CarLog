@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0start.ps1" %*
set "carlog_result=%errorlevel%"
pause
exit /b %carlog_result%
