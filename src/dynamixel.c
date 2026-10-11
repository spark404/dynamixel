#include "dynamixel/dynamixel.h"

#include <string.h>

#include "../priv_include/dynamixel/api.h"

#define DYNAMIXEL_MAX_SERVO_LIST 64

static dynamixel_result_t validate_servo(const dynamixel_servo_t *servo);

dynamixel_result_t dynamixel_init(dynamixel_servo_t *servo, uint8_t id, uint8_t type, dynamixel_bus_t *bus)
{
	if (servo == NULL || bus == NULL) {
		return DNM_API_ERR;
	}

	if (type != DYNAMIXEL_XL430) {
		return DNM_NOT_SUPPORTED;
	}

	servo->id = id;
	servo->type = type;
	servo->initialized = 1;
	servo->bus = bus;

    return DNM_OK;
}

dynamixel_result_t dynamixel_ping(const dynamixel_servo_t *servo) {
	const dynamixel_result_t res = validate_servo(servo);
	if (res != DNM_OK) {
		return res;
	}

	return dynamixel_send_ping(servo->id, servo->bus);
}

dynamixel_result_t dynamixel_set_led(const dynamixel_servo_t *servo, const uint8_t state) {
	const dynamixel_result_t res = validate_servo(servo);
	if (res != DNM_OK) {
		return res;
	}

	return dynamixel_write(servo->id, XL430_CT_RAM_LED, 1, state != 0, servo->bus);
}


dynamixel_result_t dynamixel_set_torque_enable(const dynamixel_servo_t *servo, const uint8_t state) {
	const dynamixel_result_t res = validate_servo(servo);
	if (res != DNM_OK) {
		return res;
	}

	return dynamixel_write(servo->id, XL430_CT_RAM_TORQUE_ENABLE, 1, state != 0, servo->bus);
}

dynamixel_result_t dynamixel_get_byte_parameter(const dynamixel_servo_t *servo, const uint16_t parameter, uint8_t *value) {
	const dynamixel_result_t res = validate_servo(servo);
	if (res != DNM_OK) {
		return res;
	}

	uint32_t entry_value;
	dynamixel_result_t result = dynamixel_read(servo->id, parameter, 1, &entry_value, servo->bus);

	if (result != DNM_OK) {
		return result;
	}

	*value = entry_value & 0xFF;
	return DNM_OK;
}

dynamixel_result_t dynamixel_get_word_parameter(const dynamixel_servo_t *servo, const uint16_t parameter, uint16_t *value) {
	const dynamixel_result_t res = validate_servo(servo);
	if (res != DNM_OK) {
		return res;
	}

	uint32_t entry_value;
	dynamixel_result_t result = dynamixel_read(servo->id, parameter, 2, &entry_value, servo->bus);

	if (result != DNM_OK) {
		return result;
	}

	*value = entry_value & 0xFFFF;
	return DNM_OK;
}

dynamixel_result_t dynamixel_get_long_parameter(const dynamixel_servo_t *servo, const uint16_t parameter, uint32_t *value) {
	const dynamixel_result_t res = validate_servo(servo);
	if (res != DNM_OK) {
		return res;
	}

	uint32_t entry_value;
	dynamixel_result_t result = dynamixel_read(servo->id, parameter, 4, &entry_value, servo->bus);

	if (result != DNM_OK) {
		return result;
	}

	*value = entry_value;
	return DNM_OK;
}

dynamixel_result_t dynamixel_set_byte_parameter(const dynamixel_servo_t *servo, const uint16_t parameter, const uint8_t value) {
	const dynamixel_result_t res = validate_servo(servo);
	if (res != DNM_OK) {
		return res;
	}

	return dynamixel_write(servo->id, parameter, 1, value, servo->bus);
}


dynamixel_result_t dynamixel_set_word_parameter(const dynamixel_servo_t *servo, const uint16_t parameter, const uint16_t value) {
	const dynamixel_result_t res = validate_servo(servo);
	if (res != DNM_OK) {
		return res;
	}

	return dynamixel_write(servo->id, parameter, 2, value, servo->bus);
}

dynamixel_result_t dynamixel_set_long_parameter(const dynamixel_servo_t *servo, const uint16_t parameter, const uint32_t value) {
	const dynamixel_result_t res = validate_servo(servo);
	if (res != DNM_OK) {
		return res;
	}

	return dynamixel_write(servo->id, parameter, 4, value, servo->bus);
}

/**
 * Check a servo list for a Sync Read or Write and copy its identifiers: the
 * list must not be empty or longer than DYNAMIXEL_MAX_SERVO_LIST, all servos
 * must be on the same bus and be supported. identifiers must hold
 * DYNAMIXEL_MAX_SERVO_LIST entries.
 */
static dynamixel_result_t collect_identifiers(const dynamixel_servo_t *servo_list, const size_t servo_list_size,
                                              uint8_t *identifiers) {
	if (servo_list == NULL || servo_list_size == 0 || servo_list_size > DYNAMIXEL_MAX_SERVO_LIST) {
		return DNM_API_ERR;
	}

	// Verify all servos are on the same bus
	// Verify all servos are correctly configured
	// Convert the servos to a list of identifiers
	for (size_t i = 0; i < servo_list_size; i++) {
		if (servo_list[0].bus != servo_list[i].bus) {
			return DNM_API_ERR;
		}

		const dynamixel_result_t res = validate_servo(&servo_list[i]);
		if (res != DNM_OK) {
			return res;
		}

		identifiers[i] = servo_list[i].id;
	}

	return DNM_OK;
}

dynamixel_result_t dynamixel_set_long_parameter_multiple(const dynamixel_servo_t *servo_list, const size_t servo_list_size, const uint16_t parameter, const uint32_t *values) {
	if (servo_list == NULL || values == NULL) {
		return DNM_API_ERR;
	}

	uint8_t identifiers[DYNAMIXEL_MAX_SERVO_LIST];
	const dynamixel_result_t res = collect_identifiers(servo_list, servo_list_size, identifiers);
	if (res != DNM_OK) {
		return res;
	}

	return dynamixel_sync_write(identifiers, servo_list_size, parameter, 4, values, servo_list[0].bus);
}

dynamixel_result_t dynamixel_get_long_parameter_multiple(const dynamixel_servo_t *servo_list, const size_t servo_list_size, const uint16_t parameter, uint32_t *values) {
	if (servo_list == NULL || values == NULL) {
		return DNM_API_ERR;
	}

	uint8_t identifiers[DYNAMIXEL_MAX_SERVO_LIST];
	const dynamixel_result_t res = collect_identifiers(servo_list, servo_list_size, identifiers);
	if (res != DNM_OK) {
		return res;
	}

	return dynamixel_sync_read(identifiers, servo_list_size, parameter, 4, values, servo_list[0].bus);
}

dynamixel_result_t dynamixel_set_bytes_multiple(const dynamixel_servo_t *servo_list, const size_t servo_list_size, const uint16_t parameter, const uint16_t length, const uint8_t *data) {
	if (servo_list == NULL || data == NULL) {
		return DNM_API_ERR;
	}

	uint8_t identifiers[DYNAMIXEL_MAX_SERVO_LIST];
	const dynamixel_result_t res = collect_identifiers(servo_list, servo_list_size, identifiers);
	if (res != DNM_OK) {
		return res;
	}

	return dynamixel_sync_write_bytes(identifiers, servo_list_size, parameter, length, data, servo_list[0].bus);
}

dynamixel_result_t dynamixel_get_bytes_multiple(const dynamixel_servo_t *servo_list, const size_t servo_list_size, const uint16_t parameter, const uint16_t length, uint8_t *data, dynamixel_result_t *status) {
	if (servo_list == NULL || data == NULL) {
		return DNM_API_ERR;
	}

	uint8_t identifiers[DYNAMIXEL_MAX_SERVO_LIST];
	const dynamixel_result_t res = collect_identifiers(servo_list, servo_list_size, identifiers);
	if (res != DNM_OK) {
		// servo_list_size is valid here unless it was the reason for the failure
		if (status != NULL && servo_list_size > 0 && servo_list_size <= DYNAMIXEL_MAX_SERVO_LIST) {
			memset(status, res, servo_list_size);
		}
		return res;
	}

	return dynamixel_sync_read_bytes(identifiers, servo_list_size, parameter, length, data, status, servo_list[0].bus);
}

static dynamixel_result_t validate_servo(const dynamixel_servo_t *servo) {
	if (servo == NULL || servo->bus == NULL) {
		return DNM_API_ERR;
	}

	if (servo->type != DYNAMIXEL_XL430) {
		return DNM_NOT_SUPPORTED;
	}

	return DNM_OK;
}



