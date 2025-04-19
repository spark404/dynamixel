#include "dynamixel/dynamixel.h"
#include "dynamixel/api.h"
#include "dynamixel/protocol.h"

#include <stddef.h>


dynamixel_result_t dynamixel_init(dynamixel_servo_t *servo, uint8_t id, uint8_t type, dynamixel_bus_t *bus)
{
	servo->id = id;
	servo->type = type;
	servo->initialized = 1;
	servo->bus = bus;

    return DNM_OK;
}

dynamixel_result_t dynamixel_ping(dynamixel_servo_t *servo) {
	return dynamixel_send_ping(servo->id, servo->bus);
}

dynamixel_result_t dynamixel_set_led(dynamixel_servo_t *servo, uint8_t state) {
	if (servo == NULL) {
		return DNM_API_ERR;
	}

	if (servo->type != DYNAMIXEL_XL430) {
		return DNM_NOT_SUPPORTED;
	}

	return dynamixel_write(servo->id, XL430_CT_RAM_LED, state != 0, servo->bus);
}


dynamixel_result_t dynamixel_set_torque_enable(dynamixel_servo_t *servo, uint8_t state) {
	if (servo == NULL) {
		return DNM_API_ERR;
	}

	if (servo->type != DYNAMIXEL_XL430) {
		return DNM_NOT_SUPPORTED;
	}

	return dynamixel_write(servo->id, XL430_CT_RAM_TORQUE_ENABLE, state != 0, servo->bus);
}

dynamixel_result_t dynamixel_get_byte_parameter(dynamixel_servo_t *servo, uint16_t parameter, uint8_t *value) {
	if (servo == NULL) {
		return DNM_API_ERR;
	}

	if (servo->type != DYNAMIXEL_XL430) {
		return DNM_NOT_SUPPORTED;
	}

	uint32_t entry_value;
	dynamixel_result_t result = dynamixel_read(servo->id, parameter, 1, &entry_value, servo->bus);

	if (result != DNM_OK) {
		return result;
	}

	*value = entry_value & 0xFF;
	return DNM_OK;
}

dynamixel_result_t dynamixel_get_word_parameter(dynamixel_servo_t *servo, uint16_t parameter, uint16_t *value) {
	if (servo == NULL) {
		return DNM_API_ERR;
	}

	if (servo->type != DYNAMIXEL_XL430) {
		return DNM_NOT_SUPPORTED;
	}

	uint32_t entry_value;
	dynamixel_result_t result = dynamixel_read(servo->id, parameter, 2, &entry_value, servo->bus);

	if (result != DNM_OK) {
		return result;
	}

	*value = entry_value & 0xFFFF;
	return DNM_OK;
}

dynamixel_result_t dynamixel_get_long_parameter(dynamixel_servo_t *servo, uint16_t parameter, uint32_t *value) {
	if (servo == NULL) {
		return DNM_API_ERR;
	}

	if (servo->type != DYNAMIXEL_XL430) {
		return DNM_NOT_SUPPORTED;
	}

	uint32_t entry_value;
	dynamixel_result_t result = dynamixel_read(servo->id, parameter, 4, &entry_value, servo->bus);

	if (result != DNM_OK) {
		return result;
	}

	*value = entry_value;
	return DNM_OK;
}