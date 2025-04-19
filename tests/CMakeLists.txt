include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)


add_executable(api_test api_TEST.cc)
add_dependencies(api_test dynamixel)
target_link_libraries(
        api_test
        GTest::gtest_main
        dynamixel
)

add_executable(protocol_test protocol_TEST.cc)
add_dependencies(protocol_test dynamixel)
target_link_libraries(
        protocol_test
        GTest::gtest_main
        dynamixel
)

add_executable(dynamixel_bus_test dynamixel_bus_TEST.cc)
add_dependencies(dynamixel_bus_test dynamixel)
target_link_libraries(
        dynamixel_bus_test
        GTest::gtest_main
        dynamixel
)

add_executable(dynamixel_test dynamixel_TEST.cc)
add_dependencies(dynamixel_test dynamixel)
target_link_libraries(
        dynamixel_test
        GTest::gtest_main
        dynamixel
)

include(GoogleTest)

gtest_discover_tests(
        api_test
        protocol_test
        dynamixel_bus_test
        dynamixel_test
)

add_test(NAME api_test COMMAND api_test)
add_test(NAME protocol_test COMMAND protocol_test)
add_test(NAME dynamixel_bus_test COMMAND dynamixel_bus_test)
add_test(NAME dynamixel_test COMMAND dynamixel_test)
