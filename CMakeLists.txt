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

# Optional PDF support via MuPDF (on by default).
option(WITH_PDF "Enable PDF viewing support via MuPDF" ON)
set(HAVE_MUPDF 0)
if(WITH_PDF)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(MUPDF QUIET mupdf)
    endif()
    if(MUPDF_FOUND)
        set(HAVE_MUPDF 1)
    else()
        message(WARNING "MuPDF not found; building without PDF support")
    endif()
endif()

add_executable(cuiviewer
    source-frontend/main.c
    src/settings.c
    src/thumbnails.c
    src/pdf_viewer.c
)
target_include_directories(cuiviewer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/CUI/third_party
    ${MUPDF_INCLUDE_DIRS}
)
target_compile_definitions(cuiviewer PRIVATE
    $<$<BOOL:${HAVE_MUPDF}>:HAVE_MUPDF=1>
)
target_link_libraries(cuiviewer PRIVATE cui::cui ${MUPDF_LIBRARIES})
if(MUPDF_LIBRARY_DIRS)
    target_link_directories(cuiviewer PRIVATE ${MUPDF_LIBRARY_DIRS})
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(cuiviewer PRIVATE m)
endif()

install(TARGETS cuiviewer RUNTIME DESTINATION bin)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_settings.c)
    add_executable(cuiviewer_test
        tests/test_settings.c
        src/settings.c
    )
    target_include_directories(cuiviewer_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    if(UNIX AND NOT APPLE)
        target_link_libraries(cuiviewer_test PRIVATE m)
    endif()
endif()