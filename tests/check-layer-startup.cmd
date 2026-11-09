@echo off
setlocal
call "%~dp0..\msvc32.cmd"
if errorlevel 1 exit /b 1
cd /d "%~dp0.."
cl /nologo /std:c++17 /EHsc /W4 /Ithird_party\openxr\include\openxr tests\layer-startup-unit.cpp /Fo:build\layer-startup-unit.obj /Fe:build\layer-startup-unit.exe
if errorlevel 1 exit /b 1
build\layer-startup-unit.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 tests\hold-controls-unit.cpp /Fo:build\hold-controls-unit.obj /Fe:build\hold-controls-unit.exe
if errorlevel 1 exit /b 1
build\hold-controls-unit.exe
exit /b %errorlevel%
