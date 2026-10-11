#ifndef DYNAMIXEL_PROTOCOL_H
#define DYNAMIXEL_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>


#define PING                 0x01
#define READ                 0x02
#define WRITE                0x03
#define REG_WRITE            0x04
#define ACTION               0x05
#define FACTORY_RESET        0x06
#define REBOOT               0x08
#define CLEAR                0x10
#define CONTROL_TABLE_BACKUP 0x20
#define STATUS               0x55
#define SYNC_READ            0x82
#define SYNC_WRITE           0x83
#define FAST_SYNC_READ       0x8A
#define BULK_READ            0x92
#define BULK_WRITE           0x93
#define FAST_BULK_READ       0x9A

#define STATUS_OK                   0x00
#define STATUS_RESULT_FAIL          0x01
#define STATUS_INSTRUCTION_ERROR    0x02
#define STATUS_CRC_ERROR            0x03
#define STATUS_DATA_RANGE_ERROR     0x04
#define STATUS_DATA_LENGTH_ERROR    0x05
#define STATUS_DATA_LIMIT_ERROR     0x06
#define STATUS_ACCESS_ERROR         0x07
#define STATUS_ALERT_FLAG           0x80

typedef struct __attribute__((__packed__)) {
    uint8_t header[3];
    uint8_t reserved;
    uint8_t id;
    uint16_t length;
    uint8_t instruction;
} dynamixel_packet_header_t;

typedef struct __attribute__((__packed__)) {
    uint8_t header[3];
    uint8_t reserved;
    uint8_t id;
    uint16_t length;
    uint8_t instruction;
    uint8_t error;
} dynamixel_status_packet_header_t;

#define DYNAMIXEL_PACKET_HEADER_DEFAULT { \
                                            .header = { 0xFF, 0xFF, 0xFD }, \
                                            .reserved = 0x0, \
                                            .id = 0x0, \
                                            .length = 0x0, \
                                            .instruction = 0x0 \
                                        };

typedef uint8_t dynamixel_error_t;

#define DYNAMIXEL_ERROR_NONE                     0x00
#define DYNAMIXEL_ERROR_BUFFER_TOO_SMALL         0x21
#define DYNAMIXEL_ERROR_CRC_FAIL                 0x22
#define DYNAMIXEL_ERROR_LENGTH_MISMATCH          0x23
#define DYNAMIXEL_ERROR_INVALID_STATUS_PACKET    0x24
#define DYNAMIXEL_ERROR_INVALID_PACKET           0x25

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Build an instruction packet: header, parameters and CRC.
 *
 * header.length must be param_length + 3 (instruction, CRC). The parameters are
 * byte stuffed as the protocol requires: an extra 0xFD is inserted after every
 * 0xFF 0xFF 0xFD in them, and the length in the packet (and the CRC) cover the
 * stuffed form. buffer_length must hold the stuffed packet, see
 * dynamixel_stuffed_length(); otherwise DYNAMIXEL_ERROR_BUFFER_TOO_SMALL.
 */
dynamixel_error_t dynamixel_build_packet(dynamixel_packet_header_t header, uint8_t *param, size_t param_length, uint8_t *buffer, size_t buffer_length, size_t *length);

/**
 * Number of bytes param takes once stuffed: param_length plus one for every
 * 0xFF 0xFF 0xFD in it. Never more than param_length + param_length / 3.
 */
size_t dynamixel_stuffed_length(const uint8_t *param, size_t param_length);

/**
 * Remove the stuffing from the parameters of a received packet, in place.
 * Every 0xFF 0xFF 0xFD must be followed by the stuffed 0xFD, which is dropped.
 * On success *unstuffed_length is the length of the real parameters. A sequence
 * that is not followed by 0xFD, or a stuffed byte missing at the end, returns
 * DYNAMIXEL_ERROR_INVALID_PACKET and leaves the data in an undefined state.
 * The CRC of a received packet covers the stuffed form, check it first.
 */
dynamixel_error_t dynamixel_unstuff(uint8_t *data, size_t length, size_t *unstuffed_length);

#ifdef __cplusplus
}
#endif




#endif /* DYNAMIXEL_PROTOCOL_H */
