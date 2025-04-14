@echo off

REM See INSTALL.bat for explanation of this line
PUSHD "%~dp0"

..\tcc\tcc.exe -m32 -run -lmsvcrt -luser32 -lkernel32 amb_editor.c

POPD
