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

TEST_F(DynamixelTest, GetBytesMultipleLoadVelocityPosition) {
    dynamixel_servo_t servos[2];
    dynamixel_init(&servos[0], 1, DYNAMIXEL_XL430, &this->_bus);
    dynamixel_init(&servos[1], 2, DYNAMIXEL_XL430, &this->_bus);

    const uint8_t expected_packet[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0xFE, 0x09, 0x00, 0x82, 0x7E, 0x00, 0x0A, 0x00,
        0x01, 0x02, 0x38, 0x92,
    };
    const uint8_t response[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x0E, 0x00, 0x55, 0x00, 0xDD, 0xFF, 0x64, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x13, 0x7D,
        0xFF, 0xFF, 0xFD, 0x00, 0x02, 0x0E, 0x00, 0x55, 0x00, 0x78, 0x00, 0x06, 0xFF, 0xFF, 0xFF, 0xE8, 0x03, 0x00, 0x00, 0xBD, 0x45,
    };
    prepare_response(response, sizeof(response));

    uint8_t data[2 * 10];
    dynamixel_result_t status[2];
    const dynamixel_result_t result = dynamixel_get_bytes_multiple(servos, 2, XL430_CT_RAM_PRESENT_LOAD, 10, data, status);

    ASSERT_EQ(result, DNM_OK);
    EXPECT_EQ(status[0], DNM_OK);
    EXPECT_EQ(status[1], DNM_OK);
    ASSERT_EQ(memcmp(expected_packet, _writeBuffer, sizeof(expected_packet)), 0);

    EXPECT_EQ(dynamixel_decode_i16(&data[0]), -35);
    EXPECT_EQ(dynamixel_decode_i32(&data[2]), 100);
    EXPECT_EQ(dynamixel_decode_u32(&data[6]), 2048u);
    EXPECT_EQ(dynamixel_decode_i16(&data[10]), 120);
    EXPECT_EQ(dynamixel_decode_i32(&data[12]), -250);
    EXPECT_EQ(dynamixel_decode_u32(&data[16]), 1000u);
}

TEST_F(DynamixelTest, GetBytesMultipleReportsWhichServoFailed) {
    dynamixel_servo_t servos[2];
    dynamixel_init(&servos[0], 1, DYNAMIXEL_XL430, &this->_bus);
    dynamixel_init(&servos[1], 2, DYNAMIXEL_XL430, &this->_bus);

    const uint8_t response[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x0E, 0x00, 0x55, 0x00, 0xDD, 0xFF, 0x64, 0xFF, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x13, 0x7D,
        0xFF, 0xFF, 0xFD, 0x00, 0x02, 0x0E, 0x00, 0x55, 0x00, 0x78, 0x00, 0x06, 0xFF, 0xFF, 0xFF, 0xE8, 0x03, 0x00, 0x00, 0xBD, 0x45,
    };
    prepare_response(response, sizeof(response));

    uint8_t data[2 * 10];
    memset(data, 0xEE, sizeof(data));
    dynamixel_result_t status[2];
    const dynamixel_result_t result = dynamixel_get_bytes_multiple(servos, 2, XL430_CT_RAM_PRESENT_LOAD, 10, data, status);

    EXPECT_EQ(result, DNM_RECV_CRC_FAIL);
    EXPECT_EQ(status[0], DNM_RECV_CRC_FAIL);
    EXPECT_EQ(status[1], DNM_OK);
    EXPECT_EQ(data[0], 0xEE);
    EXPECT_EQ(dynamixel_decode_i32(&data[12]), -250);
}

TEST_F(DynamixelTest, GetBytesMultipleWithoutStatus) {
    dynamixel_servo_t servos[2];
    dynamixel_init(&servos[0], 1, DYNAMIXEL_XL430, &this->_bus);
    dynamixel_init(&servos[1], 2, DYNAMIXEL_XL430, &this->_bus);

    const uint8_t response[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x0E, 0x00, 0x55, 0x00, 0xDD, 0xFF, 0x64, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x13, 0x7D,
        0xFF, 0xFF, 0xFD, 0x00, 0x02, 0x0E, 0x00, 0x55, 0x00, 0x78, 0x00, 0x06, 0xFF, 0xFF, 0xFF, 0xE8, 0x03, 0x00, 0x00, 0xBD, 0x45,
    };
    prepare_response(response, sizeof(response));

    uint8_t data[2 * 10];
    ASSERT_EQ(dynamixel_get_bytes_multiple(servos, 2, XL430_CT_RAM_PRESENT_LOAD, 10, data, nullptr), DNM_OK);
}

TEST_F(DynamixelTest, SetBytesMultipleProfileVelocityAndGoal) {
    dynamixel_servo_t servos[2];
    dynamixel_init(&servos[0], 1, DYNAMIXEL_XL430, &this->_bus);
    dynamixel_init(&servos[1], 2, DYNAMIXEL_XL430, &this->_bus);

    const uint8_t expected_packet[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0xFE, 0x19, 0x00, 0x83, 0x70, 0x00, 0x08, 0x00,
        0x01, 0x64, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x02, 0xC8, 0x00,
        0x00, 0x00, 0xE8, 0x03, 0x00, 0x00, 0xBE, 0xF4,
    };
    const uint8_t data[] = {
        0x64, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00,
        0xC8, 0x00, 0x00, 0x00, 0xE8, 0x03, 0x00, 0x00
    };

    const dynamixel_result_t result = dynamixel_set_bytes_multiple(servos, 2, XL430_CT_RAM_PROFILE_VEL, 8, data);

    ASSERT_EQ(result, DNM_OK);
    ASSERT_EQ(_writesize, sizeof(expected_packet));
    ASSERT_EQ(memcmp(expected_packet, _writeBuffer, sizeof(expected_packet)), 0);
}

TEST_F(DynamixelTest, MultipleFunctionsRejectInvalidLists) {
    dynamixel_bus_t other_bus = {};
    dynamixel_servo_t servos[2];
    dynamixel_init(&servos[0], 1, DYNAMIXEL_XL430, &this->_bus);
    dynamixel_init(&servos[1], 2, DYNAMIXEL_XL430, &this->_bus);
    dynamixel_servo_t mixed_bus[2] = {servos[0], servos[1]};
    mixed_bus[1].bus = &other_bus;
    dynamixel_servo_t unsupported[2] = {servos[0], servos[1]};
    unsupported[1].type = 2;
    dynamixel_servo_t many[254];
    for (auto &servo : many) {
        servo = servos[0];
    }

    uint8_t data[2 * 10] = {0};
    uint32_t values[2] = {0, 0};
    dynamixel_result_t status[2];

    EXPECT_EQ(dynamixel_get_bytes_multiple(nullptr, 2, 126, 10, data, status), DNM_API_ERR);
    EXPECT_EQ(dynamixel_get_bytes_multiple(servos, 0, 126, 10, data, status), DNM_API_ERR);
    EXPECT_EQ(dynamixel_get_bytes_multiple(mixed_bus, 2, 126, 10, data, status), DNM_API_ERR);
    EXPECT_EQ(dynamixel_get_bytes_multiple(unsupported, 2, 126, 10, data, status), DNM_NOT_SUPPORTED);
    EXPECT_EQ(status[0], DNM_NOT_SUPPORTED);
    EXPECT_EQ(dynamixel_get_bytes_multiple(many, 254, 126, 1, data, nullptr), DNM_API_ERR);

    EXPECT_EQ(dynamixel_set_bytes_multiple(nullptr, 2, 112, 8, data), DNM_API_ERR);
    EXPECT_EQ(dynamixel_set_bytes_multiple(servos, 0, 112, 8, data), DNM_API_ERR);
    EXPECT_EQ(dynamixel_set_bytes_multiple(mixed_bus, 2, 112, 4, data), DNM_API_ERR);
    EXPECT_EQ(dynamixel_set_bytes_multiple(unsupported, 2, 112, 4, data), DNM_NOT_SUPPORTED);

    EXPECT_EQ(dynamixel_get_long_parameter_multiple(servos, 0, 132, values), DNM_API_ERR);
    EXPECT_EQ(dynamixel_get_long_parameter_multiple(mixed_bus, 2, 132, values), DNM_API_ERR);
    EXPECT_EQ(dynamixel_set_long_parameter_multiple(servos, 0, 116, values), DNM_API_ERR);
    EXPECT_EQ(dynamixel_set_long_parameter_multiple(unsupported, 2, 116, values), DNM_NOT_SUPPORTED);

    EXPECT_EQ(_writesize, 0u) << "Nothing may be sent for an invalid list";
}

TEST_F(DynamixelTest, DecodeLittleEndian) {
    const uint8_t bytes[] = {0xDD, 0xFF, 0x2C, 0x01, 0x00, 0x80};
    EXPECT_EQ(dynamixel_decode_u16(&bytes[0]), 0xFFDDu);
    EXPECT_EQ(dynamixel_decode_i16(&bytes[0]), -35);
    EXPECT_EQ(dynamixel_decode_u32(&bytes[2]), 0x8000012Cu);
    EXPECT_EQ(dynamixel_decode_i32(&bytes[2]), (int32_t) 0x8000012C);
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