//
// Created by Hugo Trippaers on 18/04/2025.
//

#include <gtest/gtest.h>

#include "dynamixel/api.h"

class ApiTest : public testing::Test {
protected:
    dynamixel_bus_t _bus{};
    uint8_t _writeBuffer[1024]{0};
    uint8_t _readBuffer[1024]{0};
    size_t _readsize{0};
    size_t _writesize{0};

    ApiTest() {
        _bus = {
            .writeFunc = &mock_write,
            .readFunc = &mock_read,
            .flushFunc = nullptr,
            .pvContext = this
        };
    }

    ~ApiTest() override {
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
        auto *self = static_cast<ApiTest *>(pvArgument);
        return self->write(txBuffer, size);
    }

    static ssize_t mock_read(uint8_t *rxBuffer, const size_t size, void *pvArgument) {
        auto *self = static_cast<ApiTest *>(pvArgument);
        return self->read(rxBuffer, size);
    }
};

TEST_F(ApiTest, WriteByte) {
    uint8_t expected_read[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x04, 0x00, 0x55, 0x00, 0xA1, 0x0C};
    prepare_response(expected_read, sizeof(expected_read));

    dynamixel_result_t result = dynamixel_write(0x01, 132, 1, 0x11223344, &_bus);

    ASSERT_EQ(result, DNM_OK);
    ASSERT_EQ(_writesize, 13);
    ASSERT_EQ(_writeBuffer[10], 0x44);
}

TEST_F(ApiTest, WriteWord) {
    uint8_t expected_read[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x04, 0x00, 0x55, 0x00, 0xA1, 0x0C};
    prepare_response(expected_read, sizeof(expected_read));

    dynamixel_result_t result = dynamixel_write(0x01, 132, 2, 0x11223344, &_bus);

    ASSERT_EQ(result, DNM_OK);
    ASSERT_EQ(_writesize, 14);
    ASSERT_EQ(_writeBuffer[10], 0x44);
    ASSERT_EQ(_writeBuffer[11], 0x33);
}

TEST_F(ApiTest, WriteLong) {
    uint8_t expected_read[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x04, 0x00, 0x55, 0x00, 0xA1, 0x0C};
    prepare_response(expected_read, sizeof(expected_read));

    dynamixel_result_t result = dynamixel_write(0x01, 132, 4, 0x11223344, &_bus);

    ASSERT_EQ(result, DNM_OK);
    ASSERT_EQ(_writesize, 16);
    ASSERT_EQ(_writeBuffer[10], 0x44);
    ASSERT_EQ(_writeBuffer[11], 0x33);
    ASSERT_EQ(_writeBuffer[12], 0x22);
    ASSERT_EQ(_writeBuffer[13], 0x11);

}

TEST_F(ApiTest, SyncWriteLong) {
    uint8_t expected_packet[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0xFE, 0x11, 0x00, 0x83, 0x74, 0x00, 0x04, 0x00,
        0x01, 0x96, 0x00, 0x00, 0x00, 0x02, 0xAA, 0x00, 0x00, 0x00, 0x82, 0x87
    };

    uint8_t identifiers[] = { 0x01, 0x02};
    uint32_t values[] = { 0x96, 0xAA};
    dynamixel_result_t result = dynamixel_sync_write(identifiers, 2, 116, 4, values, &_bus);

    ASSERT_EQ(result, DNM_OK);

    for (size_t i = 0; i < sizeof(expected_packet); ++i) {
        EXPECT_EQ(expected_packet[i], _writeBuffer[i]) << "Expected packet differs at index " << i;
    }
}