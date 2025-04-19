//
// Created by Hugo Trippaers on 06/10/2024.
//
#include <unistd.h>

#include "dynamixel/dynamixel.h"

dynamixel_result_t dynamixel_bus_init(dynamixel_bus_t *bus, dynamixelReadFunc_t readFunc, dynamixelWriteFunc_t writeFunc, void *pvContext) {
    bus->readFunc = readFunc;
    bus->writeFunc = writeFunc;
    bus->pvContext = pvContext;

    return DNM_OK;
}

ssize_t dynamixel_bus_write(dynamixel_bus_t *bus, uint8_t *buffer, size_t buffer_length) {
    return bus->writeFunc(buffer, buffer_length, bus->pvContext);
}

ssize_t dynamixel_bus_read(dynamixel_bus_t *bus, uint8_t *buffer, size_t buffer_length) {
    return bus->readFunc(buffer, buffer_length, bus->pvContext);

}