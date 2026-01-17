#ifndef DYNAMIXEL_API_H
#define DYNAMIXEL_API_H

#include <stdint.h>
#include "dynamixel/dynamixel.h"
#include "dynamixel/protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

dynamixel_result_t dynamixel_send_ping(uint8_t identifier, const dynamixel_bus_t *bus);

dynamixel_result_t dynamixel_write(uint8_t identifier, uint16_t entry, uint8_t entry_size, uint32_t value,
                                   const dynamixel_bus_t *bus);

dynamixel_result_t dynamixel_read(uint8_t identifier, uint16_t entry, uint8_t entry_size, uint32_t *value,
                                  const dynamixel_bus_t *bus);

dynamixel_result_t dynamixel_sync_write(const uint8_t *identifiers, size_t count, uint16_t entry, uint8_t entry_size,
                                        const uint32_t *value, const dynamixel_bus_t *bus);

dynamixel_result_t dynamixel_sync_read(const uint8_t *identifiers, size_t count, uint16_t entry, uint8_t entry_size,
                                       uint32_t *values, const dynamixel_bus_t *bus);

#ifdef __cplusplus
}
#endif

#endif /* DYNAMIXEL_CRC_H */
