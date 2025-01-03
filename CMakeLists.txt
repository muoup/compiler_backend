cmake_minimum_required(VERSION 3.25)
project(compiler_backend)

set(DEBUG OFF)
set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")
list(APPEND SOURCES "main.cpp")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Enable sanitization and warnings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -Wall -Wpedantic")
    add_executable(tests ${SOURCES})
else()

add_library(compiler_backend STATIC ${SOURCES} library.cpp library.h)
set_target_properties(compiler_backend PROPERTIES PUBLIC_HEADER "library.h")
target_include_directories(${PROJECT_NAME}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
install(
        TARGETS compiler_backend
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

endif()