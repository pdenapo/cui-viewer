@echo off
cmake -S . -B build_windows -DCMAKE_BUILD_TYPE=Release -DCUI_PLATFORM=win32
cmake --build build_windows --config Release