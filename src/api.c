#include "dynamixel/dynamixel.h"
#include "dynamixel/protocol.h"
#include "../priv_include/dynamixel/api.h"

#include <string.h>

#include "dynamixel/crc.h"

#define LL_OK 0
#define LL_BUFFER_OVERFLOW (-1)
#define LL_INVALID_MAGIC (-2)

static ssize_t dynamixel_read_response(const dynamixel_bus_t *bus, uint8_t *buffer, size_t buffer_size);
static uint8_t dynamixel_parse_response(const uint8_t *buffer, size_t len, uint8_t *status);
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
    dynamixel_result_t result = dynamixel_parse_response(buffer, n, &status);
    if (result != DNM_OK) {
        return result;
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
    dynamixel_result_t result = dynamixel_parse_response(buffer, n, &status);
    if (result != DNM_OK) {
        return result;
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
    result = dynamixel_parse_response(buffer, n, &status);
    if (result != DNM_OK) {
        return result;
    }

    if (status != STATUS_OK) {
        return status;
    }

    uint8_t *parameters = NULL;
    size_t parameter_length;
    result = dynamixel_parse_parameters(buffer, n, &parameters, &parameter_length);
    if (result != DNM_OK) {
        return result;
    }

    if (parameter_length != entry_size) {
        return DNM_API_ERR;
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

dynamixel_result_t dynamixel_sync_write_bytes(const uint8_t *identifiers, const size_t count, const uint16_t entry,
                                              const uint16_t entry_size, const uint8_t *data,
                                              const dynamixel_bus_t *bus) {
    if (bus == NULL || identifiers == NULL || data == NULL || count == 0 || entry_size == 0) {
        return DNM_API_ERR;
    }

    // Parameters are the 4 byte address and size, followed by an id and the data for each servo
    const size_t slot_size = (size_t) entry_size + 1;
    if (count > (DYNAMIXEL_MAX_PACKET_SIZE - 14) / slot_size) {
        return DNM_API_ERR;
    }
    const size_t param_length = 4 + count * slot_size;

    dynamixel_packet_header_t header = DYNAMIXEL_PACKET_HEADER_DEFAULT;
    header.id = 0xFE;
    header.length = (uint16_t) (3 + param_length);
    header.instruction = SYNC_WRITE;

    uint8_t param[DYNAMIXEL_MAX_PACKET_SIZE];
    param[0] = entry & 0xFF;
    param[1] = (entry >> 8) & 0xFF;
    param[2] = entry_size & 0xFF;
    param[3] = (entry_size >> 8) & 0xFF;

    for (size_t i = 0; i < count; i++) {
        uint8_t *slot = &param[4 + i * slot_size];
        slot[0] = identifiers[i];
        memcpy(&slot[1], &data[i * entry_size], entry_size);
    }

    uint8_t buffer[DYNAMIXEL_MAX_PACKET_SIZE];
    size_t packet_length;

    const dynamixel_error_t result = dynamixel_build_packet(header, param, param_length, buffer, sizeof(buffer),
                                                            &packet_length);
    if (result != DYNAMIXEL_ERROR_NONE) {
        return DNM_API_ERR;
    }
    const ssize_t n = dynamixel_bus_write(bus, buffer, packet_length);
    if (n == 0) {
        return DNM_LL_TIMEOUT;
    }

    if (n != (ssize_t) packet_length) {
        return DNM_LL_ERR;
    }

    return DNM_OK; // This API call does not return a status result
}

dynamixel_result_t dynamixel_sync_write(const uint8_t *identifiers, const size_t count, const uint16_t entry,
                                        const uint8_t entry_size, const uint32_t *value, const dynamixel_bus_t *bus) {
    if (bus == NULL || identifiers == NULL || value == NULL || count == 0) {
        return DNM_API_ERR;
    }

    if (entry_size != 1 && entry_size != 2 && entry_size != 4) {
        return DNM_API_ERR;
    }

    // Pack the values, little endian, to the width of the entry. The byte API
    // checks that the request fits in a packet, so refuse a count that would
    // not even fit in the local staging buffer.
    uint8_t data[DYNAMIXEL_MAX_PACKET_SIZE];
    if (count > sizeof(data) / entry_size) {
        return DNM_API_ERR;
    }
    for (size_t i = 0; i < count; i++) {
        for (size_t b = 0; b < entry_size; b++) {
            data[i * entry_size + b] = (value[i] >> (8 * b)) & 0xFF;
        }
    }

    return dynamixel_sync_write_bytes(identifiers, count, entry, entry_size, data, bus);
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

    uint8_t param[128];
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
        result = dynamixel_parse_response(buffer, n, &status);
        if (result != DNM_OK) {
            return result;
        }

        if (status != STATUS_OK) {
            return status;
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

/**
 * Read one status packet from the bus into buffer.
 *
 * The 7 byte header is read first, then the rest of the packet as announced by
 * its length field. No read is ever larger than the space left in the buffer:
 * a packet that does not fit is reported as LL_BUFFER_OVERFLOW before any of
 * its payload is read.
 *
 * @return the packet length (> 0), 0 on a read timeout, LL_BUFFER_OVERFLOW or
 *         LL_INVALID_MAGIC (< 0) for a packet that cannot be accepted, or the
 *         negative value from the bus on a bus error
 */
static ssize_t dynamixel_read_response(const dynamixel_bus_t *bus, uint8_t *buffer, const size_t buffer_size) {
    if (buffer_size == 0) {
        return 0;
    }

    // Read the response header first, 7 bytes
    const size_t header_size = 7;
    if (buffer_size < header_size) {
        return LL_BUFFER_OVERFLOW;
    }

    size_t received = 0;
    while (received < header_size) {
        const ssize_t n = dynamixel_bus_read(bus, buffer + received, header_size - received);
        if (n <= 0) {
            return n;
        }
        received += (size_t) n;
    }

    // Check Magic, include reserved bit to avoid stuffing problems
    if (!(buffer[0] == 0xFF && buffer[1] == 0xFF && buffer[2] == 0xFD && buffer[3] == 0x00)) {
        return LL_INVALID_MAGIC;
    }

    // The length field (little endian) counts everything after itself
    const size_t packet_size = header_size + (size_t) (buffer[5] | (buffer[6] << 8));
    if (packet_size > buffer_size) {
        return LL_BUFFER_OVERFLOW;
    }

    // Read the remainder of the packet
    while (received < packet_size) {
        const ssize_t n = dynamixel_bus_read(bus, buffer + received, packet_size - received);
        if (n <= 0) {
            return n;
        }
        received += (size_t) n;
    }

    return (ssize_t) packet_size;
}

static dynamixel_result_t dynamixel_parse_response(const uint8_t *buffer, const size_t len, uint8_t *status) {
    if (len < 9) {
        return DNM_API_ERR;
    }

    // Check Magic
    if (!(buffer[0] == 0xFF && buffer[1] == 0xFF && buffer[2] == 0xFD)) {
        return DNM_API_ERR;
    }

    uint16_t crc = update_crc(0, buffer, len - 2);
    if (buffer[len - 2] != (crc & 0xFF) || buffer[len - 1] != ((crc >> 8) & 0xFF)) {
        return DNM_RECV_CRC_FAIL;
    };

    // Responses have type status
    if (buffer[7] != STATUS) {
        return DNM_RECV_NOT_STATUS;
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
