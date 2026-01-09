//
// Created by Hugo Trippaers on 06/10/2024.
//
#include "dynamixel/dynamixel_bus.h"

#include <stddef.h>


dynamixel_bus_result_t dynamixel_bus_init(dynamixel_bus_t *bus,
    dynamixelReadFunc_t readFunc,
    dynamixelWriteFunc_t writeFunc,
    dynamixelFlushFunc_t flushFunc,
    void *pvContext)
{
    if (bus == NULL) {
        return DNM_BUS_UNDEFINED;
    }

    bus->readFunc = readFunc;
    bus->writeFunc = writeFunc;
    bus->flushFunc = flushFunc;
    bus->pvContext = pvContext;

    return DNM_BUS_OK;
}

ssize_t dynamixel_bus_write(const dynamixel_bus_t *bus, uint8_t *buffer, size_t buffer_length) {
    if (bus->writeFunc == NULL) {
        return DNM_BUS_UNDEFINED;
    }

    return bus->writeFunc(buffer, buffer_length, bus->pvContext);
}

ssize_t dynamixel_bus_read(const dynamixel_bus_t *bus, uint8_t *buffer, size_t buffer_length) {
    if (bus->readFunc == NULL) {
        return DNM_BUS_UNDEFINED;
    }

    return bus->readFunc(buffer, buffer_length, bus->pvContext);
}

dynamixel_bus_result_t dynamixel_bus_flush(const dynamixel_bus_t *bus) {
    if (bus->flushFunc == NULL) {
        return DNM_BUS_UNDEFINED;
    }

    return bus->flushFunc(bus->pvContext);
}
