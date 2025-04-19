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

typedef struct __attribute__((__packed__)) {
    uint8_t header[3];
    uint8_t reserved[1];
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

#define DYNAMIXEL_PACKET_HEADER_DEFAULT (dynamixel_packet_header_t){{0xFF, 0xFF, 0xFD}, 0x0, 0x0, 0x0, 0x0}

typedef uint8_t dynamixel_error_t;

#define DYNAMIXEL_ERROR_NONE             0x00
#define DYNAMIXEL_ERROR_BUFFER_TOO_SMALL 0x01
#define DYNAMIXEL_ERROR_CRC_FAIL         0x02
#define DYNAMIXEL_ERROR_LENGTH_MISMATCH  0x03
#define DYNAMIXEL_ERROR_INVALID_STATUS_PACKET   0x04

#ifdef __cplusplus
extern "C" {
#endif

dynamixel_error_t dynamixel_build_packet(dynamixel_packet_header_t header, uint8_t *param, size_t param_length, uint8_t *buffer, size_t buffer_length, size_t *length);
dynamixel_error_t dynamixel_parse_status_packet(const uint8_t *buffer, size_t buffer_size, dynamixel_status_packet_header_t *header, uint8_t *param_buffer, size_t param_buffer_size, size_t *param_length);

#ifdef __cplusplus
}
#endif




#endif /* DYNAMIXEL_PROTOCOL_H */
