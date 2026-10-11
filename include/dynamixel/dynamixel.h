#ifndef DYNAMIXEL_H
#define DYNAMIXEL_H

#include <stddef.h>
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

/* Decode little endian values from the data returned by dynamixel_get_bytes_multiple() */
static inline uint16_t dynamixel_decode_u16(const uint8_t *p) {
    return (uint16_t) (p[0] | (p[1] << 8));
}

static inline int16_t dynamixel_decode_i16(const uint8_t *p) {
    return (int16_t) dynamixel_decode_u16(p);
}

static inline uint32_t dynamixel_decode_u32(const uint8_t *p) {
    return (uint32_t) p[0] | ((uint32_t) p[1] << 8) | ((uint32_t) p[2] << 16) | ((uint32_t) p[3] << 24);
}

static inline int32_t dynamixel_decode_i32(const uint8_t *p) {
    return (int32_t) dynamixel_decode_u32(p);
}

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

/**
 * Read length bytes at control table address parameter from every servo in the
 * list with one Sync Read. The data of servo_list[i] is stored at
 * data[i * length]; data must hold servo_list_size * length bytes.
 *
 * Adjacent registers can be read together, for example Present Load (126),
 * Present Velocity (128) and Present Position (132) are one 10 byte read of the
 * address 126. Use the dynamixel_decode_*() helpers to pick the values.
 *
 * status is optional (NULL) and receives one result per servo, servo_list_size
 * entries: DNM_OK, STATUS_ALERT_FLAG (hardware alert, the data is valid), or the
 * reason this servo has no data. See dynamixel_sync_read_bytes() in
 * priv_include/dynamixel/api.h for the full list. The data of a servo without
 * usable data is left unchanged. A damaged reply does not stop the read; a
 * timeout or a lost packet header does, and marks this and all following servos.
 *
 * Returns the first result that is not DNM_OK, or DNM_OK. The request must fit
 * a packet of DYNAMIXEL_MAX_PACKET_SIZE (256) bytes.
 */
dynamixel_result_t dynamixel_get_bytes_multiple(const dynamixel_servo_t *servo_list, size_t servo_list_size,
                                                uint16_t parameter, uint16_t length, uint8_t *data,
                                                dynamixel_result_t *status);

/**
 * Write length bytes at control table address parameter to every servo in the
 * list with one Sync Write. The data for servo_list[i] is taken from
 * data[i * length]. Adjacent registers can be written together, for example
 * Profile Velocity (112) and Goal Position (116) are one 8 byte write at 112.
 *
 * The request must fit a packet of DYNAMIXEL_MAX_PACKET_SIZE (256) bytes,
 * 14 + servo_list_size * (length + 1).
 */
dynamixel_result_t dynamixel_set_bytes_multiple(const dynamixel_servo_t *servo_list, size_t servo_list_size,
                                                uint16_t parameter, uint16_t length, const uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif /* DYNAMIXEL_H */
