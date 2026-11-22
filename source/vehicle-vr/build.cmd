@echo off
setlocal
call "%~dp0..\msvc32.cmd"
if errorlevel 1 exit /b 1
cd /d "%~dp0.."
if not exist build\vehicle-vr mkdir build\vehicle-vr
cl /nologo /std:c++17 /EHsc /W4 /O2 /MD /LD vehicle-vr\plugin.cpp /Fo:build\vehicle-vr\plugin.obj /Fe:build\vehicle-vr\FilthyCasualVehicles.dll /link /DEF:vehicle-vr\plugin.def /IMPLIB:build\vehicle-vr\FilthyCasualVehicles.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MD /LD /I"third_party\openxr\include\openxr" vehicle-vr\input_layer.cpp /Fo:build\vehicle-vr\input.obj /Fe:build\vehicle-vr\FilthyCasualVehiclesInput.dll /link /DEF:vehicle-vr\input.def /IMPLIB:build\vehicle-vr\input.lib
if errorlevel 1 exit /b 1
exit /b %errorlevel%
