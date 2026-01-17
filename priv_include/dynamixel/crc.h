#ifndef DYNAMIXEL_CRC_H
#define DYNAMIXEL_CRC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t update_crc(uint16_t crc_accum, const uint8_t *data_blk_ptr, uint16_t data_blk_size);

#ifdef __cplusplus
}
#endif

#endif /* DYNAMIXEL_CRC_H */
