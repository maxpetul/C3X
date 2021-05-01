@echo off

REM See INSTALL.bat for explanation of this line
PUSHD "%~dp0"

tcc\tcc.exe -m32 -I"lua/include" -run -luser32 ep.c --run

POPD
