@echo off
setlocal
call "%~dp0msvc32.cmd"
if errorlevel 1 exit /b 1
cd /d "%~dp0"
if not exist build mkdir build
cl /nologo /std:c++17 /EHsc /W4 /O2 /MD /LD plugin.cpp /Fo:build\plugin.obj /Fe:build\SeatedControls.dll /link /DEF:plugin.def /IMPLIB:build\SeatedControls.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MD /LD /I"third_party\openxr\include\openxr" input_layer.cpp /Fo:build\input_layer.obj /Fe:build\SeatedInput.dll /link /DEF:layer.def /IMPLIB:build\SeatedInput.lib
exit /b %errorlevel%
