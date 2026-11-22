@echo off
setlocal
call "%~dp0..\msvc32.cmd"
if errorlevel 1 exit /b 1
cd /d "%~dp0.."
cl /nologo /std:c++17 /EHsc /W4 tests\sticky-food-unit.cpp /Fo:build\sticky-food-unit.obj /Fe:build\sticky-food-unit.exe
if errorlevel 1 exit /b 1
build\sticky-food-unit.exe
exit /b %errorlevel%
