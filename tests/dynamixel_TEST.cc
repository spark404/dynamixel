//
// Created by Hugo Trippaers on 19/04/2025.
//
#include <dynamixel/dynamixel.h>
#include <gtest/gtest.h>

class DynamixelTest : public testing::Test {
protected:
    dynamixel_bus_t _bus{};
    uint8_t _writeBuffer[1024]{0};
    uint8_t _readBuffer[1024]{0};
    size_t _readsize{0};
    size_t _writesize{0};

    DynamixelTest() {
        _bus = {
            .writeFunc = &mock_write,
            .readFunc = &mock_read,
            .flushFunc = nullptr,
            .pvContext = this
        };
    }

    ~DynamixelTest() override {
        _bus = {nullptr, nullptr, nullptr, nullptr};
    }

    void SetUp() override {
    }

    void TearDown() override {

    }

    void prepare_response(const uint8_t *txBuffer, const size_t size) {
        memcpy(_readBuffer, txBuffer, size);
        _readsize = size;
    }

    ssize_t write(const uint8_t *txBuffer, const size_t size) {
        if (size > sizeof(_writeBuffer)) {
            return  -1;
        }
        memcpy(_writeBuffer, txBuffer, size);
        _writesize = size;
        return size;
    }

    ssize_t read(uint8_t *rxBuffer, const size_t size) {
        size_t readsize = std::min(_readsize, size);
        memcpy(rxBuffer, _readBuffer, readsize);

        _readsize -= readsize;
        memcpy(_readBuffer, _readBuffer + readsize, sizeof(_readBuffer) - readsize);

        return readsize;
    }

    static ssize_t mock_write(const uint8_t *txBuffer, const size_t size, void *pvArgument) {
        auto *self = static_cast<DynamixelTest *>(pvArgument);
        return self->write(txBuffer, size);
    }

    static ssize_t mock_read(uint8_t *rxBuffer, const size_t size, void *pvArgument) {
        auto *self = static_cast<DynamixelTest *>(pvArgument);
        return self->read(rxBuffer, size);
    }
};

TEST_F(DynamixelTest, ValidateServoNull) {
    dynamixel_result_t result = dynamixel_ping(NULL);
    ASSERT_EQ(result, DNM_API_ERR);
}

TEST_F(DynamixelTest, ValidateServoBusNull) {
    dynamixel_servo_t servo = {
        1,
        DYNAMIXEL_XL430,
        true,
        nullptr
    };

    dynamixel_result_t result = dynamixel_ping(&servo);
    ASSERT_EQ(result, DNM_API_ERR);
}

TEST_F(DynamixelTest, ValidateServoNotSupported) {
    dynamixel_bus_t bus = {};
    dynamixel_servo_t servo = {
        1,
        2,
        true,
        &bus
    };

    dynamixel_result_t result = dynamixel_ping(&servo);
    ASSERT_EQ(result, DNM_NOT_SUPPORTED);
}

TEST_F(DynamixelTest, ReadByte) {
    dynamixel_servo_t servo;
    dynamixel_init(&servo, 1, DYNAMIXEL_XL430, &this->_bus);

    uint8_t expected_write[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x07, 0x00, 0x02, 0x84, 0x00, 0x01, 0x00, 0x1D, 0x0B};
    uint8_t expected_read[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x05, 0x00, 0x55, 0x00, 0x02, 0x5C, 0xA1};
    memcpy(_readBuffer, expected_read, sizeof(expected_read));
    _readsize = sizeof(expected_read);

    uint8_t present_position;
    dynamixel_result_t result = dynamixel_get_byte_parameter(&servo, XL430_CT_RAM_PRESENT_POSITION, &present_position);

    ASSERT_EQ(result, DNM_OK);

    ASSERT_EQ(memcmp(expected_write, _writeBuffer, sizeof(expected_write)), 0);


    ASSERT_EQ(present_position, 2);
}

TEST_F(DynamixelTest, ReadWord) {
    dynamixel_servo_t servo;
    dynamixel_init(&servo, 1, DYNAMIXEL_XL430, &this->_bus);

    uint8_t expected_write[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x07, 0x00, 0x02, 0x84, 0x00, 0x02, 0x00, 0x1D, 0x01};
    uint8_t expected_read[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x06, 0x00, 0x55, 0x00, 0x02, 0x08, 0xF6, 0xD7};
    memcpy(_readBuffer, expected_read, sizeof(expected_read));
    _readsize = sizeof(expected_read);

    uint16_t present_position;
    dynamixel_result_t result = dynamixel_get_word_parameter(&servo, XL430_CT_RAM_PRESENT_POSITION, &present_position);

    ASSERT_EQ(result, DNM_OK);

    ASSERT_EQ(memcmp(expected_write, _writeBuffer, sizeof(expected_write)), 0);


    ASSERT_EQ(present_position, 2050);
}

TEST_F(DynamixelTest, ReadLong) {
    dynamixel_servo_t servo;
    dynamixel_init(&servo, 1, DYNAMIXEL_XL430, &this->_bus);

    uint8_t expected_write[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x07, 0x00, 0x02, 0x84, 0x00, 0x04, 0x00, 0x1D, 0x015};
    uint8_t expected_read[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x08, 0x00, 0x55, 0x00, 0x02, 0x08, 0x00, 0x00, 0x1F, 0x90};
    memcpy(_readBuffer, expected_read, sizeof(expected_read));
    _readsize = sizeof(expected_read);

    uint32_t present_position;
    dynamixel_result_t result = dynamixel_get_long_parameter(&servo, XL430_CT_RAM_PRESENT_POSITION, &present_position);

    ASSERT_EQ(result, DNM_OK);

    ASSERT_EQ(memcmp(expected_write, _writeBuffer, sizeof(expected_write)), 0);


    ASSERT_EQ(present_position, 2050);
}

TEST_F(DynamixelTest, SyncWriteLong) {
    dynamixel_servo_t servo_1, servo_2;
    dynamixel_init(&servo_1, 1, DYNAMIXEL_XL430, &this->_bus);
    dynamixel_init(&servo_2, 2, DYNAMIXEL_XL430, &this->_bus);

    uint8_t expected_packet[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0xFE, 0x11, 0x00, 0x83, 0x74, 0x00, 0x04, 0x00,
        0x01, 0x96, 0x00, 0x00, 0x00, 0x02, 0xAA, 0x00, 0x00, 0x00, 0x82, 0x87
    };
    dynamixel_servo_t servos[] = {servo_1, servo_2};
    uint32_t values[] = {0x96, 0xAA};
    dynamixel_result_t result = dynamixel_set_long_parameter_multiple(servos, 2, XL430_CT_RAM_GOAL_POSITION, values);

    ASSERT_EQ(result, DNM_OK);

    ASSERT_EQ(memcmp(expected_packet, _writeBuffer, sizeof(expected_packet)), 0);
}

TEST_F(DynamixelTest, SyncReadLong) {
    dynamixel_servo_t servo_1, servo_2;
    dynamixel_init(&servo_1, 1, DYNAMIXEL_XL430, &this->_bus);
    dynamixel_init(&servo_2, 2, DYNAMIXEL_XL430, &this->_bus);

    uint8_t expected_read[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x08, 0x00, 0x55, 0x00, 0xA6, 0x00, 0x00, 0x00, 0x8C, 0xC0,
        0xFF, 0xFF, 0xFD, 0x00, 0x02, 0x08, 0x00, 0x55, 0x00, 0x1F, 0x08, 0x00, 0x00, 0xBA, 0xBE,
        };
    prepare_response(expected_read, sizeof(expected_read));

    uint8_t expected_packet[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0xFE, 0x09, 0x00, 0x82, 0x84, 0x00, 0x04, 0x00,
        0x01, 0x02, 0xCE, 0xFA
    };

    dynamixel_servo_t servos[] = {servo_1, servo_2};
    uint32_t values[2];
    dynamixel_result_t result = dynamixel_get_long_parameter_multiple(servos, 2, XL430_CT_RAM_PRESENT_POSITION, values);

    ASSERT_EQ(result, DNM_OK);

    ASSERT_EQ(memcmp(expected_packet, _writeBuffer, sizeof(expected_packet)), 0);
}

TEST_F(DynamixelTest, SyncReadLongWithError) {
    dynamixel_servo_t servo_1, servo_2;
    dynamixel_init(&servo_1, 1, DYNAMIXEL_XL430, &this->_bus);
    dynamixel_init(&servo_2, 2, DYNAMIXEL_XL430, &this->_bus);

    uint8_t expected_read[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x08, 0x00, 0x55, 0x00, 0xA6, 0x00, 0x00, 0x00, 0x8C, 0xC0,
        0xFF, 0xFF, 0xFD, 0x00, 0x02, 0x08, 0x00, 0x55, 0x00, 0x1F, 0x08, 0x00, 0x00, 0xBA, 0xBE,
        };
    prepare_response(expected_read, sizeof(expected_read));

    uint8_t expected_packet[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0xFE, 0x09, 0x00, 0x82, 0x84, 0x00, 0x04, 0x00,
        0x01, 0x02, 0xCE, 0xFA
    };

    dynamixel_servo_t servos[] = {servo_1, servo_2};
    uint32_t values[2];
    dynamixel_result_t result = dynamixel_get_long_parameter_multiple(servos, 2, XL430_CT_RAM_PRESENT_POSITION, values);

    ASSERT_EQ(result, DNM_OK);

    ASSERT_EQ(memcmp(expected_packet, _writeBuffer, sizeof(expected_packet)), 0);
}

TEST_F(DynamixelTest, Ping) {
    dynamixel_servo_t servo_1, servo_2;
    dynamixel_init(&servo_1, 1, DYNAMIXEL_XL430, &this->_bus);
    dynamixel_init(&servo_2, 2, DYNAMIXEL_XL430, &this->_bus);

    uint8_t expected_read[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x07, 0x00, 0x55, 0x00, 0x06, 0x04, 0x26, 0x65, 0x5D};
    memcpy(_readBuffer, expected_read, sizeof(expected_read));
    _readsize = sizeof(expected_read);

    const uint8_t expected_packet[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x03, 0x00, 0x01, 0x19, 0x4e
    };

    dynamixel_result_t result = dynamixel_ping(&servo_1);
    ASSERT_EQ(result, DNM_OK);
    ASSERT_EQ(memcmp(expected_packet, _writeBuffer, sizeof(expected_packet)), 0);
}

TEST_F(DynamixelTest, WriteByte) {
    dynamixel_servo_t servo;
    dynamixel_init(&servo, 1, DYNAMIXEL_XL430, &this->_bus);

    const uint8_t prepared_response[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x04, 0x00, 0x55, 0x00, 0xA1, 0x0C
    };
    memcpy(_readBuffer, prepared_response, sizeof(prepared_response));
    _readsize = sizeof(prepared_response);

    const uint8_t expected_packet[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x06, 0x00, 0x03,
        0x14, 0x00, 0x0A, 0xF1, 0x62
    };

    dynamixel_result_t result = dynamixel_set_byte_parameter(&servo, XL430_CT_EEP_HOMING_OFFSET, 10);

    ASSERT_EQ(result, DNM_OK);
    ASSERT_EQ(memcmp(expected_packet, _writeBuffer, sizeof(expected_packet)), 0);
}

TEST_F(DynamixelTest, WriteWord) {
    dynamixel_servo_t servo;
    dynamixel_init(&servo, 1, DYNAMIXEL_XL430, &this->_bus);

    const uint8_t prepared_response[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x04, 0x00, 0x55, 0x00, 0xA1, 0x0C
    };
    memcpy(_readBuffer, prepared_response, sizeof(prepared_response));
    _readsize = sizeof(prepared_response);

    const uint8_t expected_packet[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x07, 0x00, 0x03,
        0x14, 0x00, 0x0A, 0x00, 0x5C, 0xF1
    };

    dynamixel_result_t result = dynamixel_set_word_parameter(&servo, XL430_CT_EEP_HOMING_OFFSET, 10);

    ASSERT_EQ(result, DNM_OK);
    ASSERT_EQ(memcmp(expected_packet, _writeBuffer, sizeof(expected_packet)), 0);
}

TEST_F(DynamixelTest, WriteLong) {
    dynamixel_servo_t servo;
    dynamixel_init(&servo, 1, DYNAMIXEL_XL430, &this->_bus);

    const uint8_t prepared_response[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x04, 0x00, 0x55, 0x00, 0xA1, 0x0C
    };
    memcpy(_readBuffer, prepared_response, sizeof(prepared_response));
    _readsize = sizeof(prepared_response);

    const uint8_t expected_packet[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x09, 0x00, 0x03,
        0x14, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x87, 0x81
    };

    dynamixel_result_t result = dynamixel_set_long_parameter(&servo, XL430_CT_EEP_HOMING_OFFSET, 10);

    ASSERT_EQ(result, DNM_OK);
    ASSERT_EQ(memcmp(expected_packet, _writeBuffer, sizeof(expected_packet)), 0);
}

TEST_F(DynamixelTest, InitNullServo) {
    dynamixel_result_t result = dynamixel_init(nullptr, 1, DYNAMIXEL_XL430, &this->_bus);
    ASSERT_EQ(result, DNM_API_ERR);
}

TEST_F(DynamixelTest, InitNullBus) {
    dynamixel_servo_t servo{};
    dynamixel_result_t result = dynamixel_init(&servo, 1, DYNAMIXEL_XL430, nullptr);
    ASSERT_EQ(result, DNM_API_ERR);
}

TEST_F(DynamixelTest, InitUnsupportedType) {
    dynamixel_servo_t servo{};
    dynamixel_result_t result = dynamixel_init(&servo, 1, 0xFF, &this->_bus);
    ASSERT_EQ(result, DNM_NOT_SUPPORTED);
}

TEST_F(DynamixelTest, InitOk) {
    dynamixel_servo_t servo{};
    dynamixel_result_t result = dynamixel_init(&servo, 5, DYNAMIXEL_XL430, &this->_bus);
    ASSERT_EQ(result, DNM_OK);
    ASSERT_EQ(servo.id, 5);
    ASSERT_EQ(servo.type, DYNAMIXEL_XL430);
    ASSERT_EQ(servo.bus, &this->_bus);
}

TEST_F(DynamixelTest, SetLongMultipleNullList) {
    uint32_t values[1] = {0};
    dynamixel_result_t result = dynamixel_set_long_parameter_multiple(
        nullptr, 1, XL430_CT_RAM_GOAL_POSITION, values);
    ASSERT_EQ(result, DNM_API_ERR);
}

TEST_F(DynamixelTest, SetLongMultipleNullValues) {
    dynamixel_servo_t servo;
    dynamixel_init(&servo, 1, DYNAMIXEL_XL430, &this->_bus);
    dynamixel_servo_t servos[] = {servo};
    dynamixel_result_t result = dynamixel_set_long_parameter_multiple(
        servos, 1, XL430_CT_RAM_GOAL_POSITION, nullptr);
    ASSERT_EQ(result, DNM_API_ERR);
}

TEST_F(DynamixelTest, SetLongMultipleSizeTooLarge) {
    dynamixel_servo_t servo;
    dynamixel_init(&servo, 1, DYNAMIXEL_XL430, &this->_bus);
    dynamixel_servo_t servos[1] = {servo};
    uint32_t values[1] = {0};
    // Size of 65 exceeds the wrapper cap (DYNAMIXEL_MAX_SERVO_LIST = 64);
    // this guards against stack-overflow via VLA usage.
    dynamixel_result_t result = dynamixel_set_long_parameter_multiple(
        servos, 65, XL430_CT_RAM_GOAL_POSITION, values);
    ASSERT_EQ(result, DNM_API_ERR);
}

TEST_F(DynamixelTest, GetLongMultipleSizeTooLarge) {
    dynamixel_servo_t servo;
    dynamixel_init(&servo, 1, DYNAMIXEL_XL430, &this->_bus);
    dynamixel_servo_t servos[1] = {servo};
    uint32_t values[1];
    dynamixel_result_t result = dynamixel_get_long_parameter_multiple(
        servos, 65, XL430_CT_RAM_PRESENT_POSITION, values);
    ASSERT_EQ(result, DNM_API_ERR);
}

TEST_F(DynamixelTest, GetLongMultipleNullList) {
    uint32_t values[1];
    dynamixel_result_t result = dynamixel_get_long_parameter_multiple(
        nullptr, 1, XL430_CT_RAM_PRESENT_POSITION, values);
    ASSERT_EQ(result, DNM_API_ERR);
}

TEST_F(DynamixelTest, GetLongMultipleNullValues) {
    dynamixel_servo_t servo;
    dynamixel_init(&servo, 1, DYNAMIXEL_XL430, &this->_bus);
    dynamixel_servo_t servos[] = {servo};
    dynamixel_result_t result = dynamixel_get_long_parameter_multiple(
        servos, 1, XL430_CT_RAM_PRESENT_POSITION, nullptr);
    ASSERT_EQ(result, DNM_API_ERR);
}

TEST_F(DynamixelTest, SetLongMultipleBusMismatch) {
    dynamixel_bus_t other_bus = {};
    dynamixel_servo_t s1, s2;
    dynamixel_init(&s1, 1, DYNAMIXEL_XL430, &this->_bus);
    dynamixel_init(&s2, 2, DYNAMIXEL_XL430, &other_bus);
    dynamixel_servo_t servos[] = {s1, s2};
    uint32_t values[] = {0x10, 0x20};
    dynamixel_result_t result = dynamixel_set_long_parameter_multiple(
        servos, 2, XL430_CT_RAM_GOAL_POSITION, values);
    ASSERT_EQ(result, DNM_API_ERR);
}

TEST_F(DynamixelTest, SetLongMultipleInvalidServoInList) {
    // First servo is initialized correctly; second is hand-built with an
    // unsupported type so validate_servo() inside the loop rejects it.
    dynamixel_servo_t s1;
    dynamixel_init(&s1, 1, DYNAMIXEL_XL430, &this->_bus);
    dynamixel_servo_t s2 = { 2, /*type=*/0xFF, /*initialized=*/1, &this->_bus };
    dynamixel_servo_t servos[] = {s1, s2};
    uint32_t values[] = {0x10, 0x20};
    dynamixel_result_t result = dynamixel_set_long_parameter_multiple(
        servos, 2, XL430_CT_RAM_GOAL_POSITION, values);
    ASSERT_EQ(result, DNM_NOT_SUPPORTED);
}

TEST_F(DynamixelTest, SetLed) {
    dynamixel_servo_t servo;
    dynamixel_init(&servo, 1, DYNAMIXEL_XL430, &this->_bus);

    const uint8_t prepared_response[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x04, 0x00, 0x55, 0x00, 0xA1, 0x0C
    };
    memcpy(_readBuffer, prepared_response, sizeof(prepared_response));
    _readsize = sizeof(prepared_response);

    dynamixel_result_t result = dynamixel_set_led(&servo, 1);
    ASSERT_EQ(result, DNM_OK);

    // Write packet should target XL430_CT_RAM_LED (65 = 0x41) with value 0x01.
    ASSERT_EQ(_writeBuffer[7], 0x03);   // WRITE instruction
    ASSERT_EQ(_writeBuffer[8], 0x41);   // address low (LED = 65)
    ASSERT_EQ(_writeBuffer[9], 0x00);
    ASSERT_EQ(_writeBuffer[10], 0x01);  // state
}

TEST_F(DynamixelTest, SetLedRejectsInvalidServo) {
    dynamixel_result_t result = dynamixel_set_led(nullptr, 1);
    ASSERT_EQ(result, DNM_API_ERR);
}

TEST_F(DynamixelTest, SetTorqueEnable) {
    dynamixel_servo_t servo;
    dynamixel_init(&servo, 1, DYNAMIXEL_XL430, &this->_bus);

    const uint8_t prepared_response[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x04, 0x00, 0x55, 0x00, 0xA1, 0x0C
    };
    memcpy(_readBuffer, prepared_response, sizeof(prepared_response));
    _readsize = sizeof(prepared_response);

    // Pass a non-zero, non-1 value to confirm the wrapper coerces it via `state != 0`.
    dynamixel_result_t result = dynamixel_set_torque_enable(&servo, 42);
    ASSERT_EQ(result, DNM_OK);

    ASSERT_EQ(_writeBuffer[7], 0x03);   // WRITE instruction
    ASSERT_EQ(_writeBuffer[8], 0x40);   // address low (TORQUE_ENABLE = 64)
    ASSERT_EQ(_writeBuffer[9], 0x00);
    ASSERT_EQ(_writeBuffer[10], 0x01);  // coerced to 1
}

TEST_F(DynamixelTest, SetTorqueEnableRejectsInvalidServo) {
    dynamixel_result_t result = dynamixel_set_torque_enable(nullptr, 0);
    ASSERT_EQ(result, DNM_API_ERR);
}