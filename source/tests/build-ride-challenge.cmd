@echo off
setlocal
call "%~dp0..\msvc32.cmd"
if errorlevel 1 exit /b 1
cd /d "%~dp0.."
cl /nologo /std:c++17 /EHsc /O2 /MD tests\ride-challenge.cpp /Fo:build\vehicle-vr\ride-test.obj /Fe:build\vehicle-vr\ride-test.exe
if errorlevel 1 exit /b 1
build\vehicle-vr\ride-test.exe
exit /b %errorlevel%
