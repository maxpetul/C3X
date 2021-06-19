@echo off

REM See INSTALL.bat for explanation of this line
PUSHD "%~dp0"

tcc\tcc.exe -m32 -Wl,-nostdlib -run -lmsvcrt -luser32 -lkernel32 -DC3X_RUN ep.c

POPD
