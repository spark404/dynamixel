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
 * Data containing 0xFF 0xFF 0xFD is byte stuffed on the wire (one extra byte
 * each), which counts towards that size.
 */
dynamixel_result_t dynamixel_sync_write_bytes(const uint8_t *identifiers, size_t count, uint16_t entry,
                                              uint16_t entry_size, const uint8_t *data, const dynamixel_bus_t *bus);

dynamixel_result_t dynamixel_sync_write(const uint8_t *identifiers, size_t count, uint16_t entry, uint8_t entry_size,
                                        const uint32_t *value, const dynamixel_bus_t *bus);

/**
 * Sync Read of entry_size bytes at control table address entry from count
 * servos. The data of identifiers[i] is stored at data[i * entry_size]; data
 * must hold count * entry_size bytes.
 *
 * Every servo gets a result, stored in status[i] when status is not NULL
 * (status must then hold count entries):
 *   DNM_OK                        the data was stored
 *   STATUS_ALERT_FLAG             the servo reports a hardware alert; its data is
 *                                 valid and was stored
 *   other servo error (1..0x87)   the servo reported an error; no data
 *   DNM_RECV_CRC_FAIL, DNM_RECV_NOT_STATUS, DNM_RECV_ID_MISMATCH,
 *   DNM_RECV_LENGTH_MISMATCH      the reply was damaged or not the one asked for
 *   DNM_LL_TIMEOUT, DNM_LL_ERR    nothing (more) could be read; this and every
 *                                 later servo is marked with it
 *
 * The data of a servo without usable data is left unchanged, so the caller can
 * keep using its previous value.
 *
 * A damaged reply does not end the read: the packet boundaries are still known,
 * so the following servos are read as usual. A timeout, a bus error, a bad
 * packet header or a reply that does not fit the buffer do end it, because the
 * stream cannot be trusted any more (after a timeout the remaining servos may
 * still be waiting for the missing reply).
 *
 * The return value is the first result that is not DNM_OK, or DNM_OK when every
 * servo was read. A request that does not fit DYNAMIXEL_MAX_PACKET_SIZE
 * (14 + count bytes out, 11 + entry_size bytes per reply) returns DNM_API_ERR.
 */
dynamixel_result_t dynamixel_sync_read_bytes(const uint8_t *identifiers, size_t count, uint16_t entry,
                                             uint16_t entry_size, uint8_t *data, dynamixel_result_t *status,
                                             const dynamixel_bus_t *bus);

dynamixel_result_t dynamixel_sync_read(const uint8_t *identifiers, size_t count, uint16_t entry, uint8_t entry_size,
                                       uint32_t *values, const dynamixel_bus_t *bus);

#ifdef __cplusplus
}
#endif

#endif /* DYNAMIXEL_CRC_H */
