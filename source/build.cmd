@echo off
setlocal
call "%~dp0msvc32.cmd"
if errorlevel 1 exit /b 1
cd /d "%~dp0"
if not exist build mkdir build
if /i "%~1"=="scope-test" goto scope_test
cl /nologo /std:c++17 /EHsc /W4 /O2 /MD /LD plugin.cpp /Fo:build\plugin.obj /Fe:build\SeatedControls.dll /link /DEF:plugin.def /IMPLIB:build\SeatedControls.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MD /LD /I"third_party\openxr\include\openxr" input_layer.cpp third_party\miniz\miniz_tinfl.c /Fo:build\ /Fe:build\SeatedInput.dll /link /DEF:layer.def /IMPLIB:build\SeatedInput.lib
if errorlevel 1 exit /b 1
if not exist build\support mkdir build\support
cl /nologo /std:c++17 /EHsc /W4 /O2 /MD /LD support\support.cpp /Fo:build\support\support.obj /Fe:build\support\000_FilthyCasualsSupport.dll /link /DEF:support\support.def /IMPLIB:build\support\support.lib advapi32.lib
exit /b %errorlevel%

:scope_test
if not exist build\scope-test mkdir build\scope-test
cl /nologo /std:c++17 /EHsc /W4 /O2 /MD /LD /DFC_SCOPE_DIAGNOSTICS plugin.cpp /Fo:build\scope-test\plugin.obj /Fe:build\scope-test\SeatedControls.dll /link /DEF:plugin.def /IMPLIB:build\scope-test\SeatedControls.lib
exit /b %errorlevel%
