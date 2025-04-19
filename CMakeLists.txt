cmake_minimum_required(VERSION 3.15...4.0)

project(
        DynamixelLibrary
        VERSION 0.1
        DESCRIPTION "A library for controlling dynamixel servos"
        LANGUAGES C CXX
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

        include(CTest)

endif()

add_subdirectory(src)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
        add_subdirectory(tests)
endif()
