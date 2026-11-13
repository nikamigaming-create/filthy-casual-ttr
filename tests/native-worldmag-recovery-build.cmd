@echo off
setlocal
cd /d "%~dp0.."
call msvc32.cmd
if errorlevel 1 exit /b 1
if not exist work\native-worldmag-recovery mkdir work\native-worldmag-recovery
cl /nologo /std:c++17 /EHsc /W4 /O2 /MD tests\native-worldmag-recovery.cpp /Fo:work\native-worldmag-recovery\checks.obj /Fe:work\native-worldmag-recovery\checks.exe bcrypt.lib
if errorlevel 1 exit /b 1
work\native-worldmag-recovery\checks.exe
exit /b %errorlevel%
