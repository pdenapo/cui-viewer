@echo off
REM Build the Windows version (native Win32 backend).
REM Requires: CMake, Ninja and a C compiler (MSVC or MinGW).
REM PDF support is optional and needs MuPDF + pkg-config; without them the
REM build succeeds with PDF support disabled.
cmake -S . -B build_windows -G Ninja -DCMAKE_BUILD_TYPE=Release -DCUI_PLATFORM=win32
if errorlevel 1 exit /b 1
cmake --build build_windows --parallel
if errorlevel 1 exit /b 1
echo Build OK: build_windows\cuiviewer.exe
