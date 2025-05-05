#include "dynamixel/dynamixel.h"
#include "dynamixel/protocol.h"
#include "dynamixel/api.h"
#include "dynamixel/crc.h"

#include <stddef.h>
#include <string.h>

dynamixel_result_t dynamixel_status_response(dynamixel_bus_t *bus, uint8_t *param_buffer, size_t param_buffer_size, size_t *length);

dynamixel_result_t dynamixel_send_ping(uint8_t identifier, dynamixel_bus_t *bus) {
	if (bus == NULL) {
		return DNM_API_ERR;
	}

	dynamixel_packet_header_t header = DYNAMIXEL_PACKET_HEADER_DEFAULT;
	header.id = identifier;
	header.instruction = PING;
	header.length = 3;

	uint8_t buffer[128];
	size_t packet_length;
	dynamixel_result_t r = dynamixel_build_packet(header, NULL, 0, buffer, sizeof(buffer), &packet_length);
	if (r != DNM_OK) {
		return r;
	}

	ssize_t n = dynamixel_bus_write(bus, buffer, packet_length);
	if (n != packet_length) {
		return DNM_API_ERR;
	}

	return dynamixel_status_response(bus, buffer, sizeof(buffer), &packet_length);
}

dynamixel_result_t dynamixel_write(uint8_t identifier, uint16_t entry, uint8_t entry_size, uint32_t value, dynamixel_bus_t *bus) {
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
	if (n != packet_length) {
		return DNM_API_ERR;
	}

	return dynamixel_status_response(bus, buffer, sizeof(buffer), &packet_length);
}

dynamixel_result_t dynamixel_read(uint8_t identifier, uint16_t entry, uint8_t entry_size, uint32_t *value, dynamixel_bus_t *bus) {
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

	dynamixel_result_t result = dynamixel_build_packet(header, param, sizeof(param), buffer, sizeof(buffer), &packet_length);
	if (result != DNM_OK) {
		return result;
	}
	ssize_t n = dynamixel_bus_write(bus, buffer, packet_length);

	if (n != packet_length) {
		return DNM_LL_ERR;
	}

	uint8_t rx_param[32];
	size_t param_length;
	result = dynamixel_status_response(bus, rx_param, sizeof(rx_param), &param_length);
	if (result != DNM_OK) {
		return result;
	}

	if (param_length == 1) {
		*value = rx_param[0];
	} else if (param_length == 2) {
		*value = rx_param[0] | rx_param[1] << 8;
	} else {
		*value = rx_param[0] | rx_param[1] << 8 | rx_param[2] << 16 | rx_param[3] << 24;
	}

	return DNM_OK;
}

dynamixel_result_t dynamixel_sync_write(uint8_t *identifiers, size_t count, uint16_t entry, uint8_t entry_size, uint32_t *value, dynamixel_bus_t *bus) {
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

	uint8_t param[64];
	param[0] = entry & 0xFF;
	param[1] = (entry >> 8) & 0xFF;
	param[2] = entry_size & 0xFF;
	param[3] = (entry_size >> 8) & 0xFF;

	for (uint8_t i = 0; i < count; i++) {
		size_t start_index = (entry_size + 1) * i;
		param[4 + start_index] = identifiers[i];
		param[4 + start_index + 1] = value[i] & 0xFF;
		param[4 + start_index + 2] = value[i] >> 8 & 0xFF;
		param[4 + start_index + 3] = value[i] >> 16 & 0xFF;
		param[4 + start_index + 4] = value[i] >> 24 & 0xFF;
	}

	uint8_t buffer[128];
	size_t packet_length;

	dynamixel_error_t result = dynamixel_build_packet(header, param, header.length - 3, buffer, sizeof(buffer), &packet_length);
	if (result != DYNAMIXEL_ERROR_NONE) {
		return DNM_API_ERR;
	}
	ssize_t n = dynamixel_bus_write(bus, buffer, packet_length);

	if (n != packet_length) {
		return DNM_LL_ERR;
	}

	return DNM_OK; // This API call does not return a status result
}

dynamixel_result_t dynamixel_status_response(dynamixel_bus_t *bus, uint8_t *param_buffer, size_t param_buffer_size, size_t *length) {
	uint8_t rxBuffer[128];

	uint8_t *rxBufferPtr = rxBuffer;
	size_t read_remaining = 7; // header size with just the length
	size_t buffer_remaining = sizeof(rxBuffer);
	while (read_remaining) {
		ssize_t n = dynamixel_bus_read(bus, rxBufferPtr, read_remaining);
		if (n < 0) {
			return DNM_API_ERR;
		}
		read_remaining -= n;
		rxBufferPtr += n;
		buffer_remaining -= n;

		if (read_remaining > buffer_remaining) {
			return DNM_API_ERR;
		}
	}

	// Check Magic, include reserved bit to avoid stuffing problems
	if (!(rxBuffer[0] == 0xFF && rxBuffer[1] == 0xFF && rxBuffer[2] == 0xFD && rxBuffer[3] == 0x00)) {
		return DNM_API_ERR;
	}

	read_remaining = rxBuffer[5] & 0xFF | rxBuffer[6] >> 8 & 0xFF;
	while (read_remaining) {
		ssize_t n = dynamixel_bus_read(bus, rxBufferPtr, read_remaining);
		if (n < 0) {
			return DNM_API_ERR;
		}
		read_remaining -= n;
		rxBufferPtr += n;
		buffer_remaining -= n;

		if (read_remaining > buffer_remaining) {
			return DNM_API_ERR;
		}
	}

	dynamixel_status_packet_header_t status_packet_header;
	size_t packet_length = (rxBuffer[5] & 0xFF | rxBuffer[6] >> 8 & 0xFF) + 7;
	dynamixel_result_t result = dynamixel_parse_status_packet(rxBuffer, packet_length, &status_packet_header, param_buffer, param_buffer_size, length);
	if (result != DNM_OK) {
		return result;
	}

	if (status_packet_header.error != STATUS_OK) {
		return status_packet_header.error;
	}

	return DNM_OK;
}