//
// Created by Hugo Trippaers on 18/04/2025.
//

#ifndef DYNAMIXEL_ERROR_H
#define DYNAMIXEL_ERROR_H

#include "stdint.h"

/* Result type for all public functions */
typedef uint8_t dynamixel_result_t;

#define DNM_OK               0
#define DNM_API_ERR         64
#define DNM_RECV_CRC_FAIL   65
#define DNM_RECV_NOT_STATUS 66
#define DNM_NOT_SUPPORTED   67
#define DNM_LL_ERR          68
#define DNM_LL_TIMEOUT      69

#endif //DYNAMIXEL_ERROR_H
