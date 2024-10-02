cmake_minimum_required(VERSION 3.29)
project(compiler_backend)

set(DEBUG ON)
set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

if(DEBUG)
    file(GLOB_RECURSE DEBUG_SOURCES "tests/*.cpp" "tests/*.hpp")
    list(APPEND SOURCES ${DEBUG_SOURCES})

    add_executable(compiler_backend ${SOURCES})
endif()

add_library(language_ir STATIC ${SOURCES} library.cpp library.h)