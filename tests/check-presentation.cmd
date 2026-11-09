@echo off
setlocal
call "%~dp0..\msvc32.cmd"
if errorlevel 1 exit /b 1
cd /d "%~dp0.."
cl /nologo /std:c++17 /EHsc /W4 /O2 /MD tests\presentation-unit.cpp /Fo:build\presentation-unit.obj /Fe:build\presentation-unit.exe
if errorlevel 1 exit /b 1
build\presentation-unit.exe
exit /b %errorlevel%
