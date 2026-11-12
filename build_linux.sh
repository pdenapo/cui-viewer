#!/bin/sh
cmake -S . -B build_linux -DCMAKE_BUILD_TYPE=Release -DCUI_PLATFORM=glfw
cmake --build build_linux -j 2