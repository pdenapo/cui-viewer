cmake_minimum_required(VERSION 3.14)
project(cuiviewer C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

if(NOT DEFINED CUI_PLATFORM OR CUI_PLATFORM STREQUAL "" OR CUI_PLATFORM STREQUAL "auto")
    if(APPLE)
        set(CUI_PLATFORM "cocoa" CACHE STRING "Window backend" FORCE)
    elseif(WIN32)
        set(CUI_PLATFORM "win32" CACHE STRING "Window backend" FORCE)
    else()
        set(CUI_PLATFORM "glfw" CACHE STRING "Window backend" FORCE)
    endif()
endif()
message(STATUS "CUI_PLATFORM: ${CUI_PLATFORM}")

if(CUI_PLATFORM STREQUAL "glfw")
    find_package(glfw3 QUIET)
    if(NOT glfw3_FOUND)
        message(FATAL_ERROR "glfw3 not found. Install it or use x11/sdl2.")
    endif()
endif()

add_subdirectory(vendor/CUI)

add_executable(cuiviewer
    source-frontend/main.c
    src/settings.c
    src/thumbnails.c
)
target_include_directories(cuiviewer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/CUI/third_party
)
target_link_libraries(cuiviewer PRIVATE cui::cui)

if(UNIX AND NOT APPLE)
    target_link_libraries(cuiviewer PRIVATE m)
endif()