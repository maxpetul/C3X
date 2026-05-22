@echo off

REM This batch file is a variation on RUN.bat that stops short of unsuspending the civ process and instead terminates it and exits successfully. It
REM also does not open any message boxes, instead printing errors to stderr. Its purpose is to verify that injected_code.c compiles and injects
REM properly without blocking for user input. It is intended to be run from the terminal or by an AI agent.

REM See INSTALL.bat for explanation of this line
PUSHD "%~dp0"

tcc\tcc.exe -m32 -Wl,-nostdlib -run -lmsvcrt -luser32 -lkernel32 -DC3X_RUN -DC3X_TEST_INJECTED_CODE_COMPILE ep.c

POPD
