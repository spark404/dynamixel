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

ssize_t dynamixel_bus_write(const dynamixel_bus_t *bus, uint8_t *buffer, size_t buffer_length) {
    if (bus->writeFunc == NULL) {
        return -1;
    }
    return bus->writeFunc(buffer, buffer_length, bus->pvContext);
}

ssize_t dynamixel_bus_read(const dynamixel_bus_t *bus, uint8_t *buffer, size_t buffer_length) {
    if (bus->readFunc == NULL) {
        return -1;
    }
    return bus->readFunc(buffer, buffer_length, bus->pvContext);
}

void dynamixel_bus_flush(const dynamixel_bus_t *bus) {
    if (bus->flushFunc != NULL) {
        bus->flushFunc(bus->pvContext);
    }
}
