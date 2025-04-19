//
// Created by Hugo Trippaers on 18/04/2025.
//

#ifndef DYNAMIXEL_BUS_H
#define DYNAMIXEL_BUS_H

#include <sys/types.h>
#include <unistd.h>

#include "dynamixel_error.h"

typedef ssize_t (*dynamixelWriteFunc_t) (const uint8_t *txBuffer, size_t size, void *pvArgument);
typedef ssize_t (*dynamixelReadFunc_t) (uint8_t *rxBuffer, size_t size, void *pvArgument);
typedef void (*dynamixelFlushFunc_t) (void *pvArgument);

typedef struct dynamixel_bus {
    dynamixelWriteFunc_t writeFunc;
    dynamixelReadFunc_t readFunc;
    dynamixelFlushFunc_t flushFunc;
    void *pvContext;
} dynamixel_bus_t;

#ifdef __cplusplus
extern "C" {
#endif

dynamixel_result_t dynamixel_bus_init(dynamixel_bus_t *bus, dynamixelReadFunc_t readFunc, dynamixelWriteFunc_t writeFunc, void *pvContext);
ssize_t dynamixel_bus_write(const dynamixel_bus_t *bus, uint8_t *buffer, size_t buffer_length);
ssize_t dynamixel_bus_read(const dynamixel_bus_t *bus, uint8_t *buffer, size_t buffer_length);
void dynamixel_bus_flush(const dynamixel_bus_t *bus);

#ifdef __cplusplus
}
#endif

#endif //DYNAMIXEL_BUS_H
