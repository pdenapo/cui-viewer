#!/usr/bin/env bash
# Build the macOS version (native Cocoa backend).
# Requires Xcode Command Line Tools. PDF support is optional and needs a
# MuPDF pkg-config file (mupdf.pc), which Homebrew's mupdf formula does not
# install; without it the build succeeds with PDF support disabled.
cmake -S . -B build_macos -DCMAKE_BUILD_TYPE=Release -DCUI_PLATFORM=cocoa
cmake --build build_macos -j 2
# ./build_macos/cuiviewer
