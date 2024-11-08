cmake_minimum_required(VERSION 3.25)
project(compiler_backend)

set(DEBUG ON)
set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    file(GLOB_RECURSE DEBUG_SOURCES "tests/*.cpp" "tests/*.hpp")
    list(APPEND SOURCES ${DEBUG_SOURCES})

    # Enable sanitization and warnings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -Wall -Wpedantic")
    add_executable(tests ${SOURCES})
else()
    add_library(language_ir STATIC ${SOURCES} library.cpp library.h)
endif()