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

/**
 * Largest packet the sync functions build or receive, in bytes. It sizes the
 * buffers on the caller's stack (two of them per call), so a bigger value costs
 * stack space in the calling task. A request that does not fit is rejected with
 * DNM_API_ERR. Can be overridden at compile time.
 */
#ifndef DYNAMIXEL_MAX_PACKET_SIZE
#define DYNAMIXEL_MAX_PACKET_SIZE 256
#endif

/**
 * Sync Write of entry_size bytes at control table address entry to count
 * servos. data holds count * entry_size bytes, the data for identifiers[i]
 * starting at data[i * entry_size].
 *
 * The packet takes 14 + count * (entry_size + 1) bytes, which must not exceed
 * DYNAMIXEL_MAX_PACKET_SIZE. Returns DNM_API_ERR for a request that does not fit.
 */
dynamixel_result_t dynamixel_sync_write_bytes(const uint8_t *identifiers, size_t count, uint16_t entry,
                                              uint16_t entry_size, const uint8_t *data, const dynamixel_bus_t *bus);

dynamixel_result_t dynamixel_sync_write(const uint8_t *identifiers, size_t count, uint16_t entry, uint8_t entry_size,
                                        const uint32_t *value, const dynamixel_bus_t *bus);

dynamixel_result_t dynamixel_sync_read(const uint8_t *identifiers, size_t count, uint16_t entry, uint8_t entry_size,
                                       uint32_t *values, const dynamixel_bus_t *bus);

#ifdef __cplusplus
}
#endif

#endif /* DYNAMIXEL_CRC_H */
