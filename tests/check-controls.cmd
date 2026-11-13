@echo off
call msvc32.cmd
cl /nologo /std:c++17 /EHsc /W4 /O2 /I. tests\controls-unit.cpp /Fo:build\controls-unit.obj /Fe:build\controls-unit.exe
if errorlevel 1 exit /b 1
build\controls-unit.exe
