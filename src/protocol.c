//
// Created by Hugo Trippaers on 18/04/2025.
//

#include "dynamixel/protocol.h"

#include <string.h>
#include "../priv_include/dynamixel/crc.h"

dynamixel_error_t dynamixel_build_packet(dynamixel_packet_header_t header, uint8_t *param, size_t param_length, uint8_t *buffer, size_t buffer_length, size_t *length) {
    if (buffer_length < sizeof(dynamixel_packet_header_t) + param_length + 2) {
        return DYNAMIXEL_ERROR_BUFFER_TOO_SMALL;
    }

    if (header.length != param_length + 3) {
        return DYNAMIXEL_ERROR_LENGTH_MISMATCH;
    }

    memcpy(buffer, &header, sizeof(dynamixel_packet_header_t));
    memcpy(buffer + sizeof(dynamixel_packet_header_t), param, param_length);

    size_t packet_length_without_crc = sizeof(dynamixel_packet_header_t) + param_length;

    uint16_t crc = update_crc(0, buffer, packet_length_without_crc);
    buffer[packet_length_without_crc] = crc & 0xFF;
    buffer[packet_length_without_crc+1] = crc >> 8 & 0xFF;

    *length = sizeof(dynamixel_packet_header_t) + param_length + 2;

    return DYNAMIXEL_ERROR_NONE;
}
