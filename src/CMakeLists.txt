set(HEADER_LIST 
        "${DynamixelLibrary_SOURCE_DIR}/include/dynamixel/api.h"
        "${DynamixelLibrary_SOURCE_DIR}/include/dynamixel/crc.h"
        "${DynamixelLibrary_SOURCE_DIR}/include/dynamixel/dynamixel.h"
        "${DynamixelLibrary_SOURCE_DIR}/include/dynamixel/dynamixel_bus.h"
        "${DynamixelLibrary_SOURCE_DIR}/include/dynamixel/dynamixel_error.h"
        "${DynamixelLibrary_SOURCE_DIR}/include/dynamixel/protocol.h"
)

add_library(dynamixel
        api.c
        crc.c
        dynamixel.c
        dynamixel_bus.c
        protocol.c
)

target_include_directories(dynamixel 
        PUBLIC ../include ../priv_include
)

# Enable coverage for Debug builds
target_compile_options(dynamixel PRIVATE "$<$<CONFIG:COVERAGE>:--coverage>")
target_link_options(dynamixel PRIVATE "$<$<CONFIG:COVERAGE>:--coverage>")

source_group(
  TREE "${PROJECT_SOURCE_DIR}/include"
  PREFIX "Header Files"
  FILES ${HEADER_LIST}
)
