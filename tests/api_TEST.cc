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

TEST_F(ApiTest, SyncReadLong) {
    uint8_t expected_packet[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0xFE, 0x09, 0x00, 0x82, 0x84, 0x00, 0x04, 0x00,
        0x01, 0x02, 0xCE, 0xFA
    };

    uint8_t expected_read[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x08, 0x00, 0x55, 0x00, 0xA6, 0x00, 0x00, 0x00, 0x8C, 0xC0,
        0xFF, 0xFF, 0xFD, 0x00, 0x02, 0x08, 0x00, 0x55, 0x00, 0x1F, 0x08, 0x00, 0x00, 0xBA, 0xBE,
        };
    prepare_response(expected_read, sizeof(expected_read));


    uint8_t identifiers[] = { 0x01, 0x02};
    uint32_t values[2];
    dynamixel_result_t result = dynamixel_sync_read(identifiers, 2, 132, 4, values, &_bus);

    ASSERT_EQ(result, DNM_OK);
    ASSERT_EQ(values[0], 166);
    ASSERT_EQ(values[1], 2079);

    for (size_t i = 0; i < sizeof(expected_packet); ++i) {
        EXPECT_EQ(expected_packet[i], _writeBuffer[i]) << "Expected packet differs at index " << i;
    }
}

TEST_F(ApiTest, Ping) {
    uint8_t expected_read[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x04, 0x00, 0x55, 0x00, 0xA1, 0x0C};
    prepare_response(expected_read, sizeof(expected_read));

    dynamixel_result_t result = dynamixel_send_ping(0x01, &_bus);

    ASSERT_EQ(result, DNM_OK);
    ASSERT_EQ(_writesize, 10);
    ASSERT_EQ(_writeBuffer[0], 0xFF);
    ASSERT_EQ(_writeBuffer[1], 0xFF);
    ASSERT_EQ(_writeBuffer[2], 0xFD);
    ASSERT_EQ(_writeBuffer[3], 0x00);
    ASSERT_EQ(_writeBuffer[4], 0x01);
    ASSERT_EQ(_writeBuffer[5], 0x03);
    ASSERT_EQ(_writeBuffer[6], 0x00);
    ASSERT_EQ(_writeBuffer[7], 0x01);
    ASSERT_EQ(_writeBuffer[8], 0x19);
    ASSERT_EQ(_writeBuffer[9], 0x4E);
}