cmake_minimum_required(VERSION 3.15...4.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(
        DynamixelLibrary
        VERSION 0.1
        DESCRIPTION "A library for controlling dynamixel servos"
        LANGUAGES C CXX
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
        add_compile_options(-Wall -Wextra -pedantic -Werror)
        include(CTest)
endif()

add_subdirectory(src)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
        add_subdirectory(tests)
endif()
