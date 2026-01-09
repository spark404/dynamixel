//
// Created by Hugo Trippaers on 18/04/2025.
//

#ifndef DYNAMIXEL_BUS_H
#define DYNAMIXEL_BUS_H

#include <stdint.h>
#include <sys/types.h>

#define DNM_BUS_OK          0
#define DNM_BUS_ERROR     (-1)
#define DNM_BUS_UNDEFINED (-2)

typedef int8_t dynamixel_bus_result_t;

typedef ssize_t (*dynamixelWriteFunc_t) (const uint8_t *txBuffer, size_t size, void *pvArgument);
typedef ssize_t (*dynamixelReadFunc_t) (uint8_t *rxBuffer, size_t size, void *pvArgument);
typedef dynamixel_bus_result_t (*dynamixelFlushFunc_t) (void *pvArgument);

typedef struct dynamixel_bus {
    dynamixelWriteFunc_t writeFunc;
    dynamixelReadFunc_t readFunc;
    dynamixelFlushFunc_t flushFunc;
    void *pvContext;
} dynamixel_bus_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialized the bus structure for a dynamixel bus
 *
 * @param bus pre-allocated dynamixel_bus_t structure
 * @param readFunc the function to read data from the bus into a buffer
 * @param writeFunc the function to write a buffer to the bus
 * @param flushFunc the function to flush the buffer
 * @param pvContext user context to be passed to read and write functions
 * @return dynamixel_bus_result_t, 0 when no errors
 */
dynamixel_bus_result_t dynamixel_bus_init(dynamixel_bus_t *bus,
    dynamixelReadFunc_t readFunc,
    dynamixelWriteFunc_t writeFunc,
    dynamixelFlushFunc_t flushFunc,
    void *pvContext);

/**
 * Write a buffer to the bus
 *
 * @param bus dynamixel_bus_t for this bus
 * @param buffer buffer containing the data to be sent
 * @param buffer_length size of the buffer
 * @return n > 0 on a succesful write,
 *         n == 0 on a write timeout,
 *         n < 0 on error
 */
ssize_t dynamixel_bus_write(const dynamixel_bus_t *bus, uint8_t *buffer, size_t buffer_length);

/**
 * Read data into a buffer from the bus
 *
 * @param bus dynamixel_bus_t for this bus
 * @param buffer a pre-allocated buffer containing the data after reading
 * @param buffer_length the size of the buffer
 * @return n > 0 n is the amount of bytes read from the bus within the timeout limit
 *         n == 0 on a read timeout without data being read
 *         n < 0 on error
 */
ssize_t dynamixel_bus_read(const dynamixel_bus_t *bus, uint8_t *buffer, size_t buffer_length);

dynamixel_bus_result_t dynamixel_bus_flush(const dynamixel_bus_t *bus);

#ifdef __cplusplus
}
#endif

#endif //DYNAMIXEL_BUS_H
