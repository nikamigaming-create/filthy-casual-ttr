@echo off
setlocal
call "%~dp0..\msvc32.cmd"
if errorlevel 1 exit /b 1
cd /d "%~dp0.."
cl /nologo /std:c++17 /EHsc /O2 /MD tests\vehicle-frame-math.cpp /Fo:build\vehicle-vr\frame-test.obj /Fe:build\vehicle-vr\frame-test.exe
if errorlevel 1 exit /b 1
build\vehicle-vr\frame-test.exe
exit /b %errorlevel%
