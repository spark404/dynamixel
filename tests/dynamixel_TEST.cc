//
// Created by Hugo Trippaers on 19/04/2025.
//
#include <dynamixel/dynamixel.h>
#include <gtest/gtest.h>

class ApiTest : public testing::Test {
protected:
    dynamixel_bus_t _bus;
    uint8_t _writeBuffer[1024]{0};
    uint8_t _readBuffer[1024]{0};
    size_t _readsize{0};

    ApiTest() {
        _bus = {
            .readFunc = &mock_read,
            .writeFunc = &mock_write,
            .pvContext = this
        };
    }

    ~ApiTest() override {
        _bus = {nullptr, nullptr, nullptr};
    }

    void SetUp() override {
    }

    void TearDown() override {

    }

    ssize_t write(const uint8_t *txBuffer, const size_t size) {
        if (size > sizeof(_writeBuffer)) {
            return  -1;
        }
        memcpy(_writeBuffer, txBuffer, size);
        return size;
    }

    ssize_t read(uint8_t *rxBuffer, const size_t size) {
        memcpy(rxBuffer, _readBuffer, _readsize);
        return _readsize;
    }

    static ssize_t mock_write(const uint8_t *txBuffer, const size_t size, void *pvArgument) {
        auto *self = static_cast<ApiTest *>(pvArgument);
        return self->write(txBuffer, size);
    }

    static ssize_t mock_read(uint8_t *rxBuffer, const size_t size, void *pvArgument) {
        auto *self = static_cast<ApiTest *>(pvArgument);
        return self->read(rxBuffer, size);
    }
};


TEST_F(ApiTest, ReadByte) {
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

TEST_F(ApiTest, ReadWord) {
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

TEST_F(ApiTest, ReadLong) {
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