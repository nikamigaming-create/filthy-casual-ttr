@echo off
setlocal
call "%~dp0..\msvc32.cmd"
if errorlevel 1 exit /b 1
cd /d "%~dp0.."
cl /nologo /std:c++17 /EHsc /O2 tests\bottle-impact.cpp /Fo:build\bottle-impact.obj /Fe:build\bottle-impact.exe
if errorlevel 1 exit /b 1
build\bottle-impact.exe
