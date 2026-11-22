@echo off
setlocal
call "%~dp0..\msvc32.cmd"
if errorlevel 1 exit /b 1
cd /d "%~dp0.."
if not exist build\eyebot mkdir build\eyebot
cl /nologo /std:c++17 /EHsc /W4 /O2 /MD /LD eyebot\plugin.cpp /Fo:build\eyebot\plugin.obj /Fe:build\eyebot\RobCoEyebot.dll /link mfplat.lib mfreadwrite.lib mfuuid.lib ole32.lib shell32.lib gdi32.lib user32.lib msimg32.lib winmm.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MD /LD /DFC_SETTINGS_ONLY eyebot\plugin.cpp /Fo:build\eyebot\native-settings.obj /Fe:build\eyebot\FilthyCasualNativeSettings.dll /link mfplat.lib mfreadwrite.lib mfuuid.lib ole32.lib shell32.lib gdi32.lib user32.lib msimg32.lib winmm.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /MD eyebot\eyebot_replay_test.cpp /Fo:build\eyebot\replay-test.obj /Fe:build\eyebot\replay-test.exe
if errorlevel 1 exit /b 1
build\eyebot\replay-test.exe
exit /b %errorlevel%
