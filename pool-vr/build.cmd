@echo off
setlocal
call "%~dp0..\msvc32.cmd"
if errorlevel 1 exit /b 1
cd /d "%~dp0.."
if not exist build\pool-vr mkdir build\pool-vr
cl /nologo /std:c++17 /EHsc /W4 /O2 /MD /LD pool-vr\plugin.cpp /Fo:build\pool-vr\plugin.obj /Fe:build\pool-vr\FilthyCasualPool.dll /link /DEF:pool-vr\plugin.def /IMPLIB:build\pool-vr\FilthyCasualPool.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MD /LD /I"third_party\openxr\include\openxr" pool-vr\input_layer.cpp /Fo:build\pool-vr\input.obj /Fe:build\pool-vr\FilthyCasualPoolInput.dll /link /DEF:pool-vr\input.def /IMPLIB:build\pool-vr\input.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MD tests\pool-cue.cpp /Fo:build\pool-vr\cue-test.obj /Fe:build\pool-vr\cue-test.exe
if errorlevel 1 exit /b 1
build\pool-vr\cue-test.exe
exit /b %errorlevel%
