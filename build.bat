@echo off
rem ===========================================================================
rem  build.bat  --  one-click build script for MinGW-w64 (g++)
rem  Usage: double-click this file, or run "build.bat" in the VSCode terminal.
rem  Note: -finput-charset / -fexec-charset keep the Chinese literals in the
rem        source files encoded as UTF-8, so console output is not garbled.
rem  (This file is ASCII-only on purpose: cmd.exe parses .bat in the OEM
rem   code page, and non-ASCII comment lines may break the script.)
rem ===========================================================================
setlocal
cd /d "%~dp0"

if not exist build mkdir build

echo [1/2] Compiling ...
g++ -std=c++17 -Wall -Wextra -g -finput-charset=UTF-8 -fexec-charset=UTF-8 main.cpp Utils.cpp BookResource.cpp Borrower.cpp Library.cpp ConsoleUI.cpp -o build\Library.exe

if errorlevel 1 goto failed

echo [2/2] Build OK:  build\Library.exe
echo Run it with:  build\Library.exe
exit /b 0

:failed
echo.
echo [BUILD FAILED] Please check the compiler messages above.
exit /b 1
