//
// Created by Hugo Trippaers on 18/04/2025.
//

#include "dynamixel/protocol.h"

#include <string.h>
#include "../priv_include/dynamixel/crc.h"

/* The sequence that has to be stuffed: 0xFF 0xFF 0xFD, which would look like a packet header */
static int ends_with_header_sequence(const uint8_t *data, const size_t index) {
    return index >= 2 && data[index] == 0xFD && data[index - 1] == 0xFF && data[index - 2] == 0xFF;
}

size_t dynamixel_stuffed_length(const uint8_t *param, const size_t param_length) {
    size_t length = param_length;
    for (size_t i = 0; i < param_length; i++) {
        if (ends_with_header_sequence(param, i)) {
            length++;
        }
    }
    return length;
}

dynamixel_error_t dynamixel_unstuff(uint8_t *data, const size_t length, size_t *unstuffed_length) {
    size_t out = 0;
    size_t in = 0;
    while (in < length) {
        data[out] = data[in];
        const int stuffed_follows = ends_with_header_sequence(data, out);
        out++;
        in++;
        if (stuffed_follows) {
            if (in >= length || data[in] != 0xFD) {
                return DYNAMIXEL_ERROR_INVALID_PACKET;
            }
            in++; // drop the stuffed byte
        }
    }
    *unstuffed_length = out;
    return DYNAMIXEL_ERROR_NONE;
}

dynamixel_error_t dynamixel_build_packet(dynamixel_packet_header_t header, uint8_t *param, size_t param_length, uint8_t *buffer, size_t buffer_length, size_t *length) {
    const size_t stuffed_length = dynamixel_stuffed_length(param, param_length);
    if (buffer_length < sizeof(dynamixel_packet_header_t) + stuffed_length + 2) {
        return DYNAMIXEL_ERROR_BUFFER_TOO_SMALL;
    }

    if (header.length != param_length + 3) {
        return DYNAMIXEL_ERROR_LENGTH_MISMATCH;
    }

    // The length in the packet counts the stuffed bytes
    header.length = (uint16_t) (stuffed_length + 3);
    memcpy(buffer, &header, sizeof(dynamixel_packet_header_t));

    size_t out = sizeof(dynamixel_packet_header_t);
    for (size_t i = 0; i < param_length; i++) {
        buffer[out++] = param[i];
        if (ends_with_header_sequence(param, i)) {
            buffer[out++] = 0xFD;
        }
    }

    uint16_t crc = update_crc(0, buffer, out);
    buffer[out] = crc & 0xFF;
    buffer[out + 1] = crc >> 8 & 0xFF;

    *length = out + 2;

    return DYNAMIXEL_ERROR_NONE;
}
