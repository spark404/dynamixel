#ifndef DYNAMIXEL_H
#define DYNAMIXEL_H

#include <stdint.h>

#include "dynamixel_error.h"
#include "dynamixel_bus.h"

/* Supported dynamixel types */
#define DYNAMIXEL_XL430 0x01

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
dynamixel_result_t dynamixel_led_set(dynamixel_servo_t *servo);
dynamixel_result_t dynamixel_led_reset(dynamixel_servo_t *servo);

#ifdef __cplusplus
}
#endif

#endif /* DYNAMIXEL_H */
