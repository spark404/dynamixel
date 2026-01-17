#ifndef DYNAMIXEL_H
#define DYNAMIXEL_H

#include <stdint.h>

#include "dynamixel_error.h"
#include "dynamixel_bus.h"

/* Supported dynamixel types */
#define DYNAMIXEL_XL430 0x01

/* Dynamixel XL430 Control Table Entries */
// EEPROM Area
#define XL430_CT_EEP_MODEL_NUMBER          0
#define XL430_CT_EEP_MODEL_INFORMATION     2
#define XL430_CT_EEP_FIRMWARE_VERSION      6
#define XL430_CT_EEP_ID                    7
#define XL430_CT_EEP_BAUD_RATE             8
#define XL430_CT_EEP_RETURN_DELAY_TIME     9
#define XL430_CT_EEP_DRIVE_MODE           10
#define XL430_CT_EEP_OPERATING_MODE       11
#define XL430_CT_EEP_SECONDARY_ID         12
#define XL430_CT_EEP_PROTOCOL_TYPE        13
#define XL430_CT_EEP_HOMING_OFFSET        20
#define XL430_CT_EEP_MOVING_THRESHOLD     24
#define XL430_CT_EEP_TEMPERATURE_LIMIT    31
#define XL430_CT_EEP_MAX_VOLTAGE_LIMIT    32
#define XL430_CT_EEP_MIN_VOLTAGE_LIMIT    34
#define XL430_CT_EEP_PWM_LIMIT            36
#define XL430_CT_EEP_VELOCITY_LIMIT       44
#define XL430_CT_EEP_MAX_POSITION_LIMIT   48
#define XL430_CT_EEP_MIN_POSITION_LIMIT   52
#define XL430_CT_EEP_SHUTDOWN             63

// RAM Area
#define XL430_CT_RAM_TORQUE_ENABLE        64
#define XL430_CT_RAM_LED                  65
#define XL430_CT_RAM_STATUS_RETURN_LEVEL  68
#define XL430_CT_RAM_REGISTERED_INST      69
#define XL430_CT_RAM_HARDWARE_ERR_STATUS  70
#define XL430_CT_RAM_VELOCITY_I_GAIN      76
#define XL430_CT_RAM_VELOCITY_P_GAIN      78
#define XL430_CT_RAM_POSITION_D_GAIN      80
#define XL430_CT_RAM_POSITION_I_GAIN      82
#define XL430_CT_RAM_POSITION_P_GAIN      84
#define XL430_CT_RAM_FEEDFORWARD_2ND_GAIN 88
#define XL430_CT_RAM_FEEDFORWARD_1ST_GAIN 90
#define XL430_CT_RAM_BUS_WATCHDOG         98
#define XL430_CT_RAM_GOAL_PWM            100
#define XL430_CT_RAM_GOAL_VELOCITY       104
#define XL430_CT_RAM_PROFILE_ACC         108
#define XL430_CT_RAM_PROFILE_VEL         112
#define XL430_CT_RAM_GOAL_POSITION       116
#define XL430_CT_RAM_REALTIME_TICK       120
#define XL430_CT_RAM_MOVING              122
#define XL430_CT_RAM_MOVING_STATUS       123
#define XL430_CT_RAM_PRESENT_PWM         124
#define XL430_CT_RAM_PRESENT_LOAD        126
#define XL430_CT_RAM_PRESENT_VELOCITY    128
#define XL430_CT_RAM_PRESENT_POSITION    132
#define XL430_CT_RAM_VELOCITY_TRAJECTORY 136
#define XL430_CT_RAM_POSITION_TRAJECTORY 140
#define XL430_CT_RAM_PRESENT_VOLTAGE     144
#define XL430_CT_RAM_PRESENT_TEMP        146

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
dynamixel_result_t dynamixel_ping(const dynamixel_servo_t *servo);
dynamixel_result_t dynamixel_set_led(const dynamixel_servo_t *servo, uint8_t state);
dynamixel_result_t dynamixel_set_torque_enable(const dynamixel_servo_t *servo, uint8_t state);

dynamixel_result_t dynamixel_get_byte_parameter(const dynamixel_servo_t *servo, uint16_t parameter, uint8_t *value);
dynamixel_result_t dynamixel_get_word_parameter(const dynamixel_servo_t *servo, uint16_t parameter, uint16_t *value);
dynamixel_result_t dynamixel_get_long_parameter(const dynamixel_servo_t *servo, uint16_t parameter, uint32_t *value);

dynamixel_result_t dynamixel_set_byte_parameter(const dynamixel_servo_t *servo, uint16_t parameter, uint8_t value);
dynamixel_result_t dynamixel_set_word_parameter(const dynamixel_servo_t *servo, uint16_t parameter, uint16_t value);
dynamixel_result_t dynamixel_set_long_parameter(const dynamixel_servo_t *servo, uint16_t parameter, uint32_t value);

dynamixel_result_t dynamixel_set_long_parameter_multiple(const dynamixel_servo_t *servo_list, size_t servo_list_size, uint16_t parameter, const uint32_t *values);
dynamixel_result_t dynamixel_get_long_parameter_multiple(const dynamixel_servo_t *servo_list, size_t servo_list_size, uint16_t parameter, uint32_t *values);

#ifdef __cplusplus
}
#endif

#endif /* DYNAMIXEL_H */
