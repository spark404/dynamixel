#ifndef DYNAMIXEL_H
#define DYNAMIXEL_H

#include <stdint.h>

#include "dynamixel_error.h"
#include "dynamixel_bus.h"

/* Supported dynamixel types */
#define DYNAMIXEL_XL430 0x01

/* Dynamixel XL430 Control Table Entries */
#define XL430_CT_EEP_OPERATING_MODE    11
#define XL430_CT_RAM_TORQUE_ENABLE     64
#define XL430_CT_RAM_LED               65
#define XL430_CT_RAM_GOAL_POSITION    116
#define XL430_CT_RAM_PRESENT_POSITION 132

typedef struct {
    uint8_t id;
    uint8_t type;
    uint8_t initialized;
    dynamixel_bus_t *bus;
} dynamixel_servo_t;

#ifdef __cplusplus
extern "C" {
#endif

dynamixel_result_t dynamixel_init(dynamixel_servo_t *servo, uint8_t id, uint8_t type, dynamixel_bus_t *bus);
dynamixel_result_t dynamixel_ping(dynamixel_servo_t *servo);
dynamixel_result_t dynamixel_set_led(dynamixel_servo_t *servo, uint8_t state);
dynamixel_result_t dynamixel_set_torque_enable(dynamixel_servo_t *servo, uint8_t state);

dynamixel_result_t dynamixel_get_byte_parameter(dynamixel_servo_t *servo, uint16_t parameter, uint8_t *value);
dynamixel_result_t dynamixel_get_word_parameter(dynamixel_servo_t *servo, uint16_t parameter, uint16_t *value);
dynamixel_result_t dynamixel_get_long_parameter(dynamixel_servo_t *servo, uint16_t parameter, uint32_t *value);

dynamixel_result_t dynamixel_set_byte_parameter(dynamixel_servo_t *servo, uint16_t parameter, uint8_t value);
dynamixel_result_t dynamixel_set_word_parameter(dynamixel_servo_t *servo, uint16_t parameter, uint16_t value);
dynamixel_result_t dynamixel_set_long_parameter(dynamixel_servo_t *servo, uint16_t parameter, uint32_t value);

dynamixel_result_t dynamixel_sync_set_long_parameter(uint16_t parameter, dynamixel_servo_t *servo, uint32_t *values, size_t count);

#ifdef __cplusplus
}
#endif

#endif /* DYNAMIXEL_H */
