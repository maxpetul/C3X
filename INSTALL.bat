@echo off

REM This line sets the current directory to the one where the file is located, ie the mod folder. This is
REM necessary in case the batch file is run as administrator with UAC enabled b/c then, for whatever reason,
REM Windows will start the script with the current directory set to C:\system32 or something like that.
PUSHD "%~dp0"

REM Unfortunately we can't use tcc -run here. See comment above low_addr_buf in ep.c for why.
tcc\tcc.exe -m32 -I"lua/include" -luser32 ep.c -o temp.exe && temp.exe --install

IF EXIST "temp.exe" (DEL temp.exe)

POPD
