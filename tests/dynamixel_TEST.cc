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
    dynamixel_result_t result = dynamixel_sync_set_long_parameter(XL430_CT_RAM_GOAL_POSITION, servos, values, 2);

    ASSERT_EQ(result, DNM_OK);

    ASSERT_EQ(memcmp(expected_packet, _writeBuffer, sizeof(expected_packet)), 0);
}