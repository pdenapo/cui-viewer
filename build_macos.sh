#!/bin/sh
cmake -S . -B build_macos -DCMAKE_BUILD_TYPE=Release -DCUI_PLATFORM=cocoa
cmake --build build_macos -j 2