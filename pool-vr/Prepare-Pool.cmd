@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Prepare-Pool.ps1"
if errorlevel 1 echo Setup failed. Read the message above; the original mod was not modified.
pause
