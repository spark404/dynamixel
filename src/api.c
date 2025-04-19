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

dynamixel_result_t dynamixel_write(uint8_t identifier, uint16_t entry, uint8_t value, dynamixel_bus_t *bus) {
	if (bus == NULL) {
		return DNM_API_ERR;
	}

	dynamixel_packet_header_t header = DYNAMIXEL_PACKET_HEADER_DEFAULT;
	header.id = identifier;
	header.instruction = WRITE;
	header.length = 6;

	// Write table number
	uint8_t param[3];
	param[0] = entry & 0xFF;
	param[1] = (entry >> 8) & 0xFF;
	param[2] = value & 0xFF;

	uint8_t buffer[128];
	size_t packet_length;
	dynamixel_error_t r = dynamixel_build_packet(header, param, 3, buffer, sizeof(buffer), &packet_length);
	if (r != DYNAMIXEL_ERROR_NONE) {
		return DNM_API_ERR;
	}

	ssize_t n = dynamixel_bus_write(bus, buffer, packet_length);
	if (n != packet_length) {
		return DNM_API_ERR;
	}

	return dynamixel_status_response(bus, buffer, sizeof(buffer), &packet_length);
}

dynamixel_result_t dynamixel_write2(uint8_t identifier, uint16_t entry, uint16_t value, dynamixel_bus_t *bus) {
	if (bus == NULL) {
		return DNM_API_ERR;
	}

	dynamixel_packet_header_t header = DYNAMIXEL_PACKET_HEADER_DEFAULT;
	header.id = identifier;
	header.instruction = WRITE;
	header.length = 7;

	// Write table number
	uint8_t param[4];
	param[0] = entry & 0xFF;
	param[1] = (entry >> 8) & 0xFF;
	param[2] = value & 0xFF;
	param[3] = (value >> 8) & 0xFF;

	uint8_t buffer[128];
	size_t packet_length;
	dynamixel_error_t r = dynamixel_build_packet(header, param, sizeof(param), buffer, sizeof(buffer), &packet_length);
	if (r != DYNAMIXEL_ERROR_NONE) {
		return DNM_API_ERR;
	}

	ssize_t n = dynamixel_bus_write(bus, buffer, packet_length);
	if (n != packet_length) {
		return DNM_API_ERR;
	}

	return dynamixel_status_response(bus, buffer, sizeof(buffer), &packet_length);
}

dynamixel_result_t dynamixel_write4(uint8_t identifier, uint16_t entry, uint32_t value, dynamixel_bus_t *bus) {
	if (bus == NULL) {
		return DNM_API_ERR;
	}

	dynamixel_packet_header_t header = DYNAMIXEL_PACKET_HEADER_DEFAULT;
	header.id = identifier;
	header.instruction = WRITE;
	header.length = 9;

	// Write table number
	uint8_t param[6];
	param[0] = entry & 0xFF;
	param[1] = (entry >> 8) & 0xFF;
	param[2] = value & 0xFF;
	param[3] = (value >> 8) & 0xFF;
	param[4] = (value >> 16) & 0xFF;
	param[5] = (value >> 24) & 0xFF;

	uint8_t buffer[128];
	size_t packet_length;
	dynamixel_error_t r = dynamixel_build_packet(header, param, sizeof(param), buffer, sizeof(buffer), &packet_length);
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

dynamixel_result_t dynamixel_status_response(dynamixel_bus_t *bus, uint8_t *param_buffer, size_t param_buffer_size, size_t *length) {
	uint8_t rxBuffer[128];
	ssize_t n = dynamixel_bus_read(bus, rxBuffer, sizeof(rxBuffer));
	if (n < 0) {
		return DNM_API_ERR;
	}

	dynamixel_status_packet_header_t status_packet_header;
	dynamixel_result_t result = dynamixel_parse_status_packet(rxBuffer, n, &status_packet_header, param_buffer, param_buffer_size, length);
	if (result != DNM_OK) {
		return result;
	}

	if (status_packet_header.error != STATUS_OK) {
		return status_packet_header.error;
	}

	return DNM_OK;
}