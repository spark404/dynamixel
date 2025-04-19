#ifndef DYNAMIXEL_API_H
#define DYNAMIXEL_API_H

#include <stdint.h>
#include "dynamixel.h"
#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

dynamixel_result_t dynamixel_send_ping(uint8_t identifier, dynamixel_bus_t *bus);

dynamixel_result_t dynamixel_write(uint8_t identifier, uint16_t entry, uint8_t entry_size, uint32_t value, dynamixel_bus_t *bus);
dynamixel_result_t dynamixel_read(uint8_t identifier, uint16_t entry, uint8_t entry_size, uint32_t *value, dynamixel_bus_t *bus);


#ifdef __cplusplus
}
#endif

#endif /* DYNAMIXEL_CRC_H */
