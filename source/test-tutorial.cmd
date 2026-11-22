@echo off
call "%~dp0msvc32.cmd"
if errorlevel 1 exit /b 1
cd /d "%~dp0"
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT tests\hold-controls-unit.cpp /Fo:build\hold-controls-unit.obj /Fe:build\hold-controls-unit.exe
if errorlevel 1 exit /b 1
build\hold-controls-unit.exe
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT tests\tutorial-data-unit.cpp /Fo:build\tutorial-data-unit.obj /Fe:build\tutorial-data-unit.exe
if errorlevel 1 exit /b 1
build\tutorial-data-unit.exe %*
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT tests\tutorial-model-unit.cpp /Fo:build\tutorial-model-unit.obj /Fe:build\tutorial-model-unit.exe
if errorlevel 1 exit /b 1
build\tutorial-model-unit.exe
cl /nologo /std:c++17 /EHsc /W4 /O2 /MT tests\ttr11-eject-state-unit.cpp /Fo:build\ttr11-eject-state-unit.obj /Fe:build\ttr11-eject-state-unit.exe
if errorlevel 1 exit /b 1
build\ttr11-eject-state-unit.exe
