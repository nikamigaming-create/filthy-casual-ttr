@echo off
setlocal
call "%~dp0msvc32.cmd"
if errorlevel 1 exit /b 1
cd /d "%~dp0"
if not exist build\physical-workshop mkdir build\physical-workshop
cl /nologo /std:c++17 /EHsc /W4 /O2 /MD /LD workshop-plugin.cpp /Fo:build\physical-workshop\plugin.obj /Fe:build\physical-workshop\FilthyCasualPhysicalWorkshop.dll /link shell32.lib
exit /b %errorlevel%
