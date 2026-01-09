#include "dynamixel/dynamixel.h"
#include "dynamixel/protocol.h"
#include "dynamixel/api.h"

#include <string.h>

static ssize_t dynamixel_read_response(const dynamixel_bus_t *bus, uint8_t *buffer, size_t len);
static uint8_t dynamixel_parse_status(const uint8_t *buffer, size_t len, uint8_t *status);
static dynamixel_result_t dynamixel_parse_parameters(uint8_t *buffer, size_t len, uint8_t **parameters, size_t *length);

dynamixel_result_t dynamixel_send_ping(const uint8_t identifier, const dynamixel_bus_t *bus) {
    if (bus == NULL) {
        return DNM_API_ERR;
    }

    dynamixel_packet_header_t header = DYNAMIXEL_PACKET_HEADER_DEFAULT;
    header.id = identifier;
    header.instruction = PING;
    header.length = 3;

    uint8_t buffer[128];
    size_t packet_length;
    if (dynamixel_build_packet(header, NULL, 0, buffer, sizeof(buffer), &packet_length) != DYNAMIXEL_ERROR_NONE) {
        return DNM_API_ERR;
    }

    ssize_t n = dynamixel_bus_write(bus, buffer, packet_length);
    if (n == 0) {
        return DNM_LL_TIMEOUT;
    }

    if (n != (ssize_t) packet_length) {
        return DNM_LL_ERR;
    }

    n = dynamixel_read_response(bus, buffer, sizeof(buffer));
    if (n == 0) {
        return DNM_LL_TIMEOUT;
    }

    if (n < 0) {
        return DNM_LL_ERR;
    }

    uint8_t status;
    if (dynamixel_parse_status(buffer, n, &status) != DNM_OK) {
        return DNM_API_ERR;
    }

    return status;
}

dynamixel_result_t dynamixel_write(const uint8_t identifier, const uint16_t entry, const uint8_t entry_size,
                                   const uint32_t value, const dynamixel_bus_t *bus) {
    if (bus == NULL) {
        return DNM_API_ERR;
    }

    if (!(entry_size == 1 || entry_size == 2 || entry_size == 4)) {
        return DNM_API_ERR;
    }

    dynamixel_packet_header_t header = DYNAMIXEL_PACKET_HEADER_DEFAULT;
    header.id = identifier;
    header.instruction = WRITE;
    header.length = 5 + entry_size;

    // Write table number
    uint8_t param[6];
    param[0] = entry & 0xFF;
    param[1] = (entry >> 8) & 0xFF;
    param[2] = value & 0xFF;
    if (entry_size >= 2) {
        param[3] = value >> 8 & 0xFF;
    }
    if (entry_size == 4) {
        param[4] = value >> 16 & 0xFF;
        param[5] = value >> 24 & 0xFF;
    }

    uint8_t buffer[128];
    size_t packet_length;
    dynamixel_error_t r = dynamixel_build_packet(header, param, 2 + entry_size, buffer, sizeof(buffer), &packet_length);
    if (r != DYNAMIXEL_ERROR_NONE) {
        return DNM_API_ERR;
    }

    ssize_t n = dynamixel_bus_write(bus, buffer, packet_length);
    if (n == 0) {
        return DNM_LL_TIMEOUT;
    }

    if (n != (ssize_t) packet_length) {
        return DNM_API_ERR;
    }

    n = dynamixel_read_response(bus, buffer, sizeof(buffer));
    if (n == 0) {
        return DNM_LL_TIMEOUT;
    }

    if (n < 0) {
        return DNM_LL_ERR;
    }

    uint8_t status;
    if (dynamixel_parse_status(buffer, n, &status) != DNM_OK) {
        return DNM_API_ERR;
    }

    return status;
}

dynamixel_result_t dynamixel_read(const uint8_t identifier, const uint16_t entry, const uint8_t entry_size,
                                  uint32_t *value, const dynamixel_bus_t *bus) {
    if (bus == NULL) {
        return DNM_API_ERR;
    }

    dynamixel_packet_header_t header = DYNAMIXEL_PACKET_HEADER_DEFAULT;
    header.id = identifier;
    header.length = 7;
    header.instruction = READ;

    if (entry_size != 1 && entry_size != 2 && entry_size != 4) {
        return DNM_API_ERR;
    }

    // Write table number
    uint8_t param[4];
    param[0] = entry & 0xFF;
    param[1] = (entry >> 8) & 0xFF;
    param[2] = entry_size & 0xFF;
    param[3] = (entry_size >> 8) & 0xFF;

    uint8_t buffer[128];
    size_t packet_length;

    dynamixel_result_t result = dynamixel_build_packet(header, param, sizeof(param), buffer, sizeof(buffer),
                                                       &packet_length);
    if (result != DNM_OK) {
        return result;
    }
    ssize_t n = dynamixel_bus_write(bus, buffer, packet_length);

    if (n == 0) {
        return DNM_LL_TIMEOUT;
    }

    if (n != (ssize_t) packet_length) {
        return DNM_LL_ERR;
    }

    n = dynamixel_read_response(bus, buffer, sizeof(buffer));
    if (n == 0) {
        return DNM_LL_TIMEOUT;
    }

    if (n < 0) {
        return DNM_LL_ERR;
    }

    uint8_t status;
    if (dynamixel_parse_status(buffer, n, &status) != DNM_OK) {
        return DNM_API_ERR;
    }

    uint8_t *parameters = NULL;
    size_t parameter_length;
    result = dynamixel_parse_parameters(buffer, n, &parameters, &parameter_length);
    if (result != DNM_OK) {
        return result;
    }

    if (parameter_length == 1) {
        *value = parameters[0];
    } else if (parameter_length == 2) {
        *value = parameters[0] | parameters[1] << 8;
    } else {
        *value = parameters[0] | parameters[1] << 8 | parameters[2] << 16 | parameters[3] << 24;
    }

    return DNM_OK;
}

dynamixel_result_t dynamixel_sync_write(const uint8_t *identifiers, const size_t count, const uint16_t entry,
                                        const uint8_t entry_size, const uint32_t *value, const dynamixel_bus_t *bus) {
    if (bus == NULL) {
        return DNM_API_ERR;
    }

    dynamixel_packet_header_t header = DYNAMIXEL_PACKET_HEADER_DEFAULT;
    header.id = 0xFE;
    header.length = 7 + count * (entry_size + 1);
    header.instruction = SYNC_WRITE;

    if (entry_size != 1 && entry_size != 2 && entry_size != 4) {
        return DNM_API_ERR;
    }

    uint8_t param[256];
    if ((entry_size + 1) * count + 4 > sizeof(param)) {
        return DNM_API_ERR;
    }

    param[0] = entry & 0xFF;
    param[1] = (entry >> 8) & 0xFF;
    param[2] = entry_size & 0xFF;
    param[3] = (entry_size >> 8) & 0xFF;

    for (size_t i = 0; i < count; i++) {
        const size_t start_index = (entry_size + 1) * i;
        param[4 + start_index] = identifiers[i];
        param[4 + start_index + 1] = value[i] & 0xFF;
        param[4 + start_index + 2] = value[i] >> 8 & 0xFF;
        param[4 + start_index + 3] = value[i] >> 16 & 0xFF;
        param[4 + start_index + 4] = value[i] >> 24 & 0xFF;
    }

    uint8_t buffer[256];
    size_t packet_length;

    dynamixel_error_t result = dynamixel_build_packet(header, param, header.length - 3, buffer, sizeof(buffer),
                                                      &packet_length);
    if (result != DYNAMIXEL_ERROR_NONE) {
        return DNM_API_ERR;
    }
    ssize_t n = dynamixel_bus_write(bus, buffer, packet_length);
    if (n == 0) {
        return DNM_LL_TIMEOUT;
    }

    if (n != (ssize_t) packet_length) {
        return DNM_LL_ERR;
    }

    return DNM_OK; // This API call does not return a status result
}

dynamixel_result_t dynamixel_sync_read(const uint8_t *identifiers, const size_t count, const uint16_t entry,
                                       const uint8_t entry_size, uint32_t *values, const dynamixel_bus_t *bus) {
    if (bus == NULL) {
        return DNM_API_ERR;
    }
    dynamixel_packet_header_t header = DYNAMIXEL_PACKET_HEADER_DEFAULT;
    header.id = 0xFE;
    header.length = 7 + count;
    header.instruction = SYNC_READ;

    if (entry_size != 1 && entry_size != 2 && entry_size != 4) {
        return DNM_API_ERR;
    }

    uint8_t param[64];
    if (entry_size * count + 4 > sizeof(param)) {
        return DNM_API_ERR;
    }

    param[0] = entry & 0xFF;
    param[1] = (entry >> 8) & 0xFF;
    param[2] = entry_size & 0xFF;
    param[3] = (entry_size >> 8) & 0xFF;

    for (size_t i = 0; i < count; i++) {
        param[4 + i] = identifiers[i];
    }

    uint8_t buffer[128];
    size_t packet_length;

    dynamixel_error_t result = dynamixel_build_packet(header, param, header.length - 3, buffer, sizeof(buffer),
                                                      &packet_length);
    if (result != DYNAMIXEL_ERROR_NONE) {
        return DNM_API_ERR;
    }
    ssize_t n = dynamixel_bus_write(bus, buffer, packet_length);
    if (n == 0) {
        return DNM_LL_TIMEOUT;
    }

    if (n != (ssize_t) packet_length) {
        return DNM_LL_ERR;
    }

    for (size_t i = 0; i < count; i++) {
        n = dynamixel_read_response(bus, buffer, sizeof(buffer));
        if (n == 0) {
            return DNM_LL_TIMEOUT;
        }

        if (n < 0) {
            return DNM_LL_ERR;
        }

        uint8_t status;
        if (dynamixel_parse_status(buffer, n, &status) != DNM_OK) {
            return DNM_API_ERR;
        }

        uint8_t *parameters = NULL;
        size_t parameter_length;
        result = dynamixel_parse_parameters(buffer, n, &parameters, &parameter_length);
        if (result != DNM_OK) {
            return result;
        }

        if (parameter_length == 1) {
            values[i] = parameters[0];
        } else if (parameter_length == 2) {
            values[i] = parameters[0] | parameters[1] << 8;
        } else {
            values[i] = parameters[0] | parameters[1] << 8 | parameters[2] << 16 | parameters[3] << 24;
        }
    }

    return DNM_OK; // This API call does not return a status result
}

static ssize_t dynamixel_read_response(const dynamixel_bus_t *bus, uint8_t *buffer, const size_t len) {
    if (len == 0) {
        return 0;
    }

    // Read the response header first, 7 bytes
    uint8_t *rxBufferPtr = buffer;
    size_t read_remaining = 7;
    size_t buffer_remaining = len;
    while (read_remaining) {
        const ssize_t n = dynamixel_bus_read(bus, rxBufferPtr, read_remaining);
        if (n <= 0) {
            return n;
        }
        read_remaining -= n;
        rxBufferPtr += n;
        buffer_remaining -= n;

        if (read_remaining > buffer_remaining) {
            return -1;
        }
    }

    // Check Magic, include reserved bit to avoid stuffing problems
    if (!(buffer[0] == 0xFF && buffer[1] == 0xFF && buffer[2] == 0xFD && buffer[3] == 0x00)) {
        return -1;
    }

    // Read the remainder of the packet
    read_remaining = (buffer[5] & 0xFF) | (buffer[6] >> 8 & 0xFF);
    buffer_remaining = len - 7;
    while (read_remaining) {
        const ssize_t n = dynamixel_bus_read(bus, rxBufferPtr, read_remaining);
        if (n <= 0) {
            return n;
        }

        read_remaining -= n;
        rxBufferPtr += n;
        buffer_remaining -= n;

        if (read_remaining > buffer_remaining) {
            return DNM_API_ERR;
        }
    }

    // return the actual length of the packet
    return ((buffer[5] & 0xFF) | (buffer[6] >> 8 & 0xFF)) + 7;
}

static dynamixel_result_t dynamixel_parse_status(const uint8_t *buffer, const size_t len, uint8_t *status) {
    if (len < 9) {
        return DNM_API_ERR;
    }

    if (buffer[7] != 0x55) {
        // Not a status packet
        return DNM_API_ERR;
    }

    *status = buffer[8];
    return DNM_OK;
}

static dynamixel_result_t dynamixel_parse_parameters(uint8_t *buffer, const size_t len, uint8_t **parameters, size_t *length) {
    if (len < 9) {
        return DNM_API_ERR;
    }

    *parameters = &buffer[9];
    *length = len - 9 - 2;
    return DNM_OK;
}
