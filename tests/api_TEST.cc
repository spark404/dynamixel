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
    ssize_t _readsize{0};
    ssize_t _writesize{0};

    // Optional overrides used to exercise bus error paths.
    // Write override: when active, mock_write returns _forceWriteReturn instead
    // of `size`. Lets tests simulate write timeout (0) or partial write (N<size).
    bool _forceWriteOverride{false};
    ssize_t _forceWriteReturn{0};
    // Read override: when active, mock_read returns _forceReadReturn directly,
    // starting from the (_forceReadAfterCalls+1)th call. Lets tests simulate
    // over-reads (n > size) or errors mid-packet without consuming the buffer.
    bool _forceReadOverride{false};
    ssize_t _forceReadReturn{0};
    int _forceReadAfterCalls{0};
    int _readCallCount{0};

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

    void prepare_response(const uint8_t *txBuffer, const ssize_t size) {
        memcpy(_readBuffer, txBuffer, size);
        _readsize = size;
    }

    ssize_t write(const uint8_t *txBuffer, const size_t size) {
        if (_writesize < 0) {
            return _writesize;
        }

        if (size > sizeof(_writeBuffer)) {
            return  -1;
        }
        memcpy(_writeBuffer, txBuffer, size);
        _writesize = size;
        if (_forceWriteOverride) {
            return _forceWriteReturn;
        }
        return size;
    }

    ssize_t read(uint8_t *rxBuffer, const ssize_t size) {
        const int call = _readCallCount++;
        if (_forceReadOverride && call >= _forceReadAfterCalls) {
            return _forceReadReturn;
        }

        if (_readsize <= 0) {
            return _readsize;
        }

        ssize_t readsize = std::min(_readsize, size);
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

TEST_F(ApiTest, WriteInvalidEntrySize) {
    dynamixel_result_t result = dynamixel_write(0x01, 132, 8, 0x11223344, &_bus);

    ASSERT_EQ(result, DNM_API_ERR);
}

TEST_F(ApiTest, WriteBusNull) {
    dynamixel_result_t result = dynamixel_write(0x01, 132, 4, 0x11223344, nullptr);

    ASSERT_EQ(result, DNM_API_ERR);
}

TEST_F(ApiTest, WriteHardwareAlert) {
    uint8_t expected_read[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x04, 0x00, 0x55, 0x80, 0xA2, 0x8F};
    prepare_response(expected_read, sizeof(expected_read));

    dynamixel_result_t result = dynamixel_write(0x01, 132, 1, 0x11223344, &_bus);

    ASSERT_EQ(result, STATUS_ALERT_FLAG);
}

TEST_F(ApiTest, WriteInvalidPacket) {
    uint8_t expected_read[] = {0xFF, 0x77, 0xFD, 0x00, 0x01, 0x04, 0x00, 0x55, 0x80, 0xA2, 0x8F};
    prepare_response(expected_read, sizeof(expected_read));

    dynamixel_result_t result = dynamixel_write(0x01, 132, 1, 0x11223344, &_bus);

    // Returns LL_ERR as the read routine will fail due to being unable to read the packet length
    // and discards the packet fragment
    ASSERT_EQ(result, DNM_LL_ERR);
}


TEST_F(ApiTest, ReadByte) {
    const uint8_t expected_sent[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x07, 0x00, 0x02,
        0x84, 0x00, 0x01, 0x00, 0x1D, 0x0B
    };
    const uint8_t response[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x05, 0x00, 0x55,
        0x00, 0xA6, 0x87, 0x22
    };
    prepare_response(response, sizeof(response));
    uint32_t value;

    dynamixel_result_t result = dynamixel_read(0x01, 132, 1, &value, &_bus);

    ASSERT_EQ(result, DNM_OK);
    ASSERT_EQ(_writesize, 14);
    ASSERT_EQ(memcmp(_writeBuffer, expected_sent, _writesize), 0);

    ASSERT_EQ(value, 166);
}

TEST_F(ApiTest, ReadWord) {
    const uint8_t expected_sent[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x07, 0x00, 0x02,
        0x84, 0x00, 0x02, 0x00, 0x1D, 0x01
    };
    const uint8_t response[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x06, 0x00, 0x55,
        0x00, 0xA6, 0x02, 0xC3, 0x8F
    };
    prepare_response(response, sizeof(response));
    uint32_t value;

    dynamixel_result_t result = dynamixel_read(0x01, 132, 2, &value, &_bus);

    ASSERT_EQ(result, DNM_OK);
    ASSERT_EQ(_writesize, 14);
    ASSERT_EQ(memcmp(_writeBuffer, expected_sent, _writesize), 0);

    ASSERT_EQ(value, 678);
}

TEST_F(ApiTest, ReadLong) {
    const uint8_t expected_sent[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x07, 0x00, 0x02,
        0x84, 0x00, 0x04, 0x00, 0x1D, 0x15
    };
    const uint8_t response[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x08, 0x00, 0x55,
        0x00, 0xA6, 0x00, 0x00, 0x00, 0x8C, 0xC0
    };
    prepare_response(response, sizeof(response));
    uint32_t value;

    dynamixel_result_t result = dynamixel_read(0x01, 132, 4, &value, &_bus);

    ASSERT_EQ(result, DNM_OK);
    ASSERT_EQ(_writesize, 14);
    ASSERT_EQ(memcmp(_writeBuffer, expected_sent, _writesize), 0);

    ASSERT_EQ(value, 166);
}

TEST_F(ApiTest, ReadHardwareAlert) {
    uint8_t expected_read[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x04, 0x00, 0x55, 0x80, 0xA2, 0x8F};
    prepare_response(expected_read, sizeof(expected_read));

    uint32_t value;

    dynamixel_result_t result = dynamixel_read(0x01, 132, 1, &value, &_bus);

    ASSERT_EQ(result, STATUS_ALERT_FLAG);
}

TEST_F(ApiTest, ReadBusNull) {
    uint32_t value;
    dynamixel_result_t result = dynamixel_read(0x01, 132, 1, &value, nullptr);

    ASSERT_EQ(result, DNM_API_ERR);
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
        0xFF, 0xFF, 0xFD, 0x00, 0xFE, 0x19, 0x00, 0x82,
        0x84, 0x00, 0x04, 0x00, 0x10, 0x11, 0x12, 0x0A,
        0x0B, 0x0C, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
        0x0D, 0x0E, 0x0F, 0x01, 0x02, 0x03, 0x1E, 0x57
    };

    uint8_t expected_read[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x10, 0x08, 0x00, 0x55, 0x00, 0x92, 0x07, 0x00, 0x00, 0x8A, 0x30,
        0xFF, 0xFF, 0xFD, 0x00, 0x11, 0x08, 0x00, 0x55, 0x00, 0xF7, 0x0B, 0x00, 0x00, 0x08, 0xF2,
        0xFF, 0xFF, 0xFD, 0x00, 0x12, 0x08, 0x00, 0x55, 0x00, 0xFE, 0x0C, 0x00, 0x00, 0xC7, 0xCC,
        0xFF, 0xFF, 0xFD, 0x00, 0x0A, 0x08, 0x00, 0x55, 0x00, 0x1B, 0x06, 0x00, 0x00, 0x61, 0x5D,
        0xFF, 0xFF, 0xFD, 0x00, 0x0B, 0x08, 0x00, 0x55, 0x00, 0xC4, 0x0A, 0x00, 0x00, 0xD6, 0x57,
        0xFF, 0xFF, 0xFD, 0x00, 0x0C, 0x08, 0x00, 0x55, 0x00, 0x7B, 0x0B, 0x00, 0x00, 0xD7, 0xC8,
        0xFF, 0xFF, 0xFD, 0x00, 0x04, 0x08, 0x00, 0x55, 0x00, 0x17, 0x08, 0x00, 0x00, 0xF9, 0x8B,
        0xFF, 0xFF, 0xFD, 0x00, 0x05, 0x08, 0x00, 0x55, 0x00, 0x91, 0x0A, 0x00, 0x00, 0x8E, 0x75,
        0xFF, 0xFF, 0xFD, 0x00, 0x06, 0x08, 0x00, 0x55, 0x00, 0x2D, 0x0B, 0x00, 0x00, 0x0C, 0x4F,
        0xFF, 0xFF, 0xFD, 0x00, 0x07, 0x08, 0x00, 0x55, 0x00, 0x7C, 0x09, 0x00, 0x00, 0x5C, 0x1D,
        0xFF, 0xFF, 0xFD, 0x00, 0x08, 0x08, 0x00, 0x55, 0x00, 0x2A, 0x0D, 0x00, 0x00, 0x34, 0x05,
        0xFF, 0xFF, 0xFD, 0x00, 0x09, 0x08, 0x00, 0x55, 0x00, 0xFD, 0x0C, 0x00, 0x00, 0x67, 0xAF,
        0xFF, 0xFF, 0xFD, 0x00, 0x0D, 0x08, 0x00, 0x55, 0x00, 0x82, 0x08, 0x00, 0x00, 0xA0, 0x3A,
        0xFF, 0xFF, 0xFD, 0x00, 0x0E, 0x08, 0x00, 0x55, 0x00, 0xD1, 0x0C, 0x00, 0x00, 0x48, 0x4C,
        0xFF, 0xFF, 0xFD, 0x00, 0x0F, 0x08, 0x00, 0x55, 0x00, 0x27, 0x0D, 0x00, 0x00, 0x14, 0xF2,
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x08, 0x00, 0x55, 0x00, 0x80, 0x08, 0x00, 0x00, 0x23, 0xB8,
        0xFF, 0xFF, 0xFD, 0x00, 0x02, 0x08, 0x00, 0x55, 0x00, 0x18, 0x0C, 0x00, 0x00, 0xEA, 0xD2,
        0xFF, 0xFF, 0xFD, 0x00, 0x03, 0x08, 0x00, 0x55, 0x00, 0xCC, 0x0C, 0x00, 0x00, 0xAE, 0xC4,
        };
    prepare_response(expected_read, sizeof(expected_read));


    uint8_t identifiers[] = {
        0x10, 0x11, 0x12, 0x0A, 0x0B, 0x0C, 0x04, 0x05,
        0x06, 0x07, 0x08, 0x09, 0x0D, 0x0E, 0x0F, 0x01,
        0x02, 0x03};
    uint32_t values[18];
    dynamixel_result_t result = dynamixel_sync_read(identifiers, 18, 132, 4, values, &_bus);

    ASSERT_EQ(result, DNM_OK);
    ASSERT_EQ(values[0], 1938);
    ASSERT_EQ(values[1], 3063);
    ASSERT_EQ(values[17], 3276);

    for (size_t i = 0; i < sizeof(expected_packet); ++i) {
        EXPECT_EQ(expected_packet[i], _writeBuffer[i]) << "Expected packet differs at index " << i;
    }
}

TEST_F(ApiTest, SyncReadLongCRCError) {
    uint8_t expected_packet[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0xFE, 0x19, 0x00, 0x82,
        0x84, 0x00, 0x04, 0x00, 0x10, 0x11, 0x12, 0x0A,
        0x0B, 0x0C, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
        0x0D, 0x0E, 0x0F, 0x01, 0x02, 0x03, 0x1E, 0x57
    };

    uint8_t expected_read[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x10, 0x08, 0x00, 0x55, 0x00, 0x92, 0x07, 0x00, 0x00, 0x8A, 0x30,
        0xFF, 0xFF, 0xFD, 0x00, 0x11, 0x08, 0x00, 0x55, 0x00, 0xF7, 0x0B, 0x00, 0x00, 0x08, 0xF2,
        0xFF, 0xFF, 0xFD, 0x00, 0x12, 0x08, 0x00, 0x55, 0x00, 0xFE, 0x0C, 0x00, 0x00, 0xC7, 0xCC,
        0xFF, 0xFF, 0xFD, 0x00, 0x0A, 0x08, 0x00, 0x55, 0x00, 0x1B, 0x06, 0x00, 0x00, 0x61, 0x5D,
        0xFF, 0xFF, 0xFD, 0x00, 0x0B, 0x08, 0x00, 0x55, 0x00, 0xC4, 0x0A, 0x00, 0x00, 0xD6, 0x57,
        0xFF, 0xFF, 0xFD, 0x00, 0x0C, 0x08, 0x00, 0x55, 0x00, 0x7B, 0x0B, 0x00, 0x00, 0xD7, 0xC8,
        0xFF, 0xFF, 0xFD, 0x00, 0x04, 0x08, 0x00, 0x55, 0x00, 0x17, 0x08, 0x00, 0x00, 0xF9, 0x8B,
        0xFF, 0xFF, 0xFD, 0x00, 0x05, 0x08, 0x00, 0x55, 0x00, 0x91, 0x0A, 0x00, 0x00, 0x8E, 0x75,
        0xFF, 0xFF, 0xFD, 0x00, 0x06, 0x08, 0x00, 0x55, 0x00, 0x2D, 0x0B, 0x00, 0x00, 0x0C, 0x4F,
        0xFF, 0xFF, 0xFD, 0x00, 0x07, 0x08, 0x00, 0x55, 0x00, 0x7C, 0x09, 0x00, 0x00, 0x5C, 0x1D,
        0xFF, 0xFF, 0xFD, 0x00, 0x08, 0x08, 0x00, 0x55, 0x00, 0x2A, 0x0D, 0x00, 0x00, 0xAA, 0xAA,  // CRC Fail
        0xFF, 0xFF, 0xFD, 0x00, 0x09, 0x08, 0x00, 0x55, 0x00, 0xFD, 0x0C, 0x00, 0x00, 0x67, 0xAF,
        0xFF, 0xFF, 0xFD, 0x00, 0x0D, 0x08, 0x00, 0x55, 0x00, 0x82, 0x08, 0x00, 0x00, 0xA0, 0x3A,
        0xFF, 0xFF, 0xFD, 0x00, 0x0E, 0x08, 0x00, 0x55, 0x00, 0xD1, 0x0C, 0x00, 0x00, 0x48, 0x4C,
        0xFF, 0xFF, 0xFD, 0x00, 0x0F, 0x08, 0x00, 0x55, 0x00, 0x27, 0x0D, 0x00, 0x00, 0x14, 0xF2,
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x08, 0x00, 0x55, 0x00, 0x80, 0x08, 0x00, 0x00, 0x23, 0xB8,
        0xFF, 0xFF, 0xFD, 0x00, 0x02, 0x08, 0x00, 0x55, 0x00, 0x18, 0x0C, 0x00, 0x00, 0xEA, 0xD2,
        0xFF, 0xFF, 0xFD, 0x00, 0x03, 0x08, 0x00, 0x55, 0x00, 0xCC, 0x0C, 0x00, 0x00, 0xAE, 0xC4,
        };
    prepare_response(expected_read, sizeof(expected_read));


    uint8_t identifiers[] = {
        0x10, 0x11, 0x12, 0x0A, 0x0B, 0x0C, 0x04, 0x05,
        0x06, 0x07, 0x08, 0x09, 0x0D, 0x0E, 0x0F, 0x01,
        0x02, 0x03};
    uint32_t values[18];
    dynamixel_result_t result = dynamixel_sync_read(identifiers, 18, 132, 4, values, &_bus);

    ASSERT_EQ(result, DNM_RECV_CRC_FAIL);

    for (size_t i = 0; i < sizeof(expected_packet); ++i) {
        EXPECT_EQ(expected_packet[i], _writeBuffer[i]) << "Expected packet differs at index " << i;
    }
}

TEST_F(ApiTest, SyncReadLongHardwareAlert) {
    uint8_t expected_packet[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0xFE, 0x19, 0x00, 0x82,
        0x84, 0x00, 0x04, 0x00, 0x10, 0x11, 0x12, 0x0A,
        0x0B, 0x0C, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
        0x0D, 0x0E, 0x0F, 0x01, 0x02, 0x03, 0x1E, 0x57
    };

    uint8_t expected_read[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x10, 0x08, 0x00, 0x55, 0x00, 0x92, 0x07, 0x00, 0x00, 0x8A, 0x30,
        0xFF, 0xFF, 0xFD, 0x00, 0x11, 0x08, 0x00, 0x55, 0x00, 0xF7, 0x0B, 0x00, 0x00, 0x08, 0xF2,
        0xFF, 0xFF, 0xFD, 0x00, 0x12, 0x08, 0x00, 0x55, 0x00, 0xFE, 0x0C, 0x00, 0x00, 0xC7, 0xCC,
        0xFF, 0xFF, 0xFD, 0x00, 0x0A, 0x08, 0x00, 0x55, 0x00, 0x1B, 0x06, 0x00, 0x00, 0x61, 0x5D,
        0xFF, 0xFF, 0xFD, 0x00, 0x0B, 0x08, 0x00, 0x55, 0x00, 0xC4, 0x0A, 0x00, 0x00, 0xD6, 0x57,
        0xFF, 0xFF, 0xFD, 0x00, 0x0C, 0x08, 0x00, 0x55, 0x00, 0x7B, 0x0B, 0x00, 0x00, 0xD7, 0xC8,
        0xFF, 0xFF, 0xFD, 0x00, 0x04, 0x08, 0x00, 0x55, 0x00, 0x17, 0x08, 0x00, 0x00, 0xF9, 0x8B,
        0xFF, 0xFF, 0xFD, 0x00, 0x05, 0x08, 0x00, 0x55, 0x00, 0x91, 0x0A, 0x00, 0x00, 0x8E, 0x75,
        0xFF, 0xFF, 0xFD, 0x00, 0x06, 0x08, 0x00, 0x55, 0x00, 0x2D, 0x0B, 0x00, 0x00, 0x0C, 0x4F,
        0xFF, 0xFF, 0xFD, 0x00, 0x07, 0x08, 0x00, 0x55, 0x00, 0x7C, 0x09, 0x00, 0x00, 0x5C, 0x1D,
        0xFF, 0xFF, 0xFD, 0x00, 0x08, 0x08, 0x00, 0x55, 0x00, 0x2A, 0x0D, 0x00, 0x00, 0x34, 0x05,
        0xFF, 0xFF, 0xFD, 0x00, 0x09, 0x08, 0x00, 0x55, 0x00, 0xFD, 0x0C, 0x00, 0x00, 0x67, 0xAF,
        0xFF, 0xFF, 0xFD, 0x00, 0x0D, 0x08, 0x00, 0x55, 0x80, 0x82, 0x08, 0x00, 0x00, 0xA3, 0x86, // HW Alert
        0xFF, 0xFF, 0xFD, 0x00, 0x0E, 0x08, 0x00, 0x55, 0x00, 0xD1, 0x0C, 0x00, 0x00, 0x48, 0x4C,
        0xFF, 0xFF, 0xFD, 0x00, 0x0F, 0x08, 0x00, 0x55, 0x00, 0x27, 0x0D, 0x00, 0x00, 0x14, 0xF2,
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x08, 0x00, 0x55, 0x00, 0x80, 0x08, 0x00, 0x00, 0x23, 0xB8,
        0xFF, 0xFF, 0xFD, 0x00, 0x02, 0x08, 0x00, 0x55, 0x00, 0x18, 0x0C, 0x00, 0x00, 0xEA, 0xD2,
        0xFF, 0xFF, 0xFD, 0x00, 0x03, 0x08, 0x00, 0x55, 0x00, 0xCC, 0x0C, 0x00, 0x00, 0xAE, 0xC4,
        };
    prepare_response(expected_read, sizeof(expected_read));


    uint8_t identifiers[] = {
        0x10, 0x11, 0x12, 0x0A, 0x0B, 0x0C, 0x04, 0x05,
        0x06, 0x07, 0x08, 0x09, 0x0D, 0x0E, 0x0F, 0x01,
        0x02, 0x03};
    uint32_t values[18];
    dynamixel_result_t result = dynamixel_sync_read(identifiers, 18, 132, 4, values, &_bus);

    ASSERT_EQ(result, STATUS_ALERT_FLAG);

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

TEST_F(ApiTest, ResponseHardwareAlert) {
    uint8_t expected_read[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x04, 0x00, 0x55, 0x80, 0xA2, 0x8F};
    prepare_response(expected_read, sizeof(expected_read));

    dynamixel_result_t result = dynamixel_send_ping(0x01, &_bus);

    ASSERT_EQ(result, STATUS_ALERT_FLAG);
}

TEST_F(ApiTest, ResponseCrcFail) {
    uint8_t expected_read[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x04, 0x00, 0x55, 0x80, 0x12, 0x34};
    prepare_response(expected_read, sizeof(expected_read));

    dynamixel_result_t result = dynamixel_send_ping(0x01, &_bus);

    ASSERT_EQ(result, DNM_RECV_CRC_FAIL);
}

TEST_F(ApiTest, BusReadTimeout) {
    _readsize = 0; // bus read returning 0 indicates timeout

    dynamixel_result_t result = dynamixel_send_ping(0x01, &_bus);

    ASSERT_EQ(result, DNM_LL_TIMEOUT);
}

TEST_F(ApiTest, BusReadError) {
    _readsize = -1; // bus read returning <0 indicates error

    dynamixel_result_t result = dynamixel_send_ping(0x01, &_bus);

    ASSERT_EQ(result, DNM_LL_ERR);
}

TEST_F(ApiTest, BusWriteError) {
    _writesize = -1; // bus write returning <0 indicates timeout

    dynamixel_result_t result = dynamixel_send_ping(0x01, &_bus);

    ASSERT_EQ(result, DNM_LL_ERR);
}

// ---------------------------------------------------------------------------
// Bus-interaction error coverage. Each public API function must map every
// possible bus_write / bus_read return code to a defined dynamixel_result_t.
// ---------------------------------------------------------------------------

// dynamixel_send_ping: write timeout
TEST_F(ApiTest, PingWriteTimeout) {
    _forceWriteOverride = true;
    _forceWriteReturn = 0;
    dynamixel_result_t result = dynamixel_send_ping(0x01, &_bus);
    ASSERT_EQ(result, DNM_LL_TIMEOUT);
}

// dynamixel_send_ping: write returns a partial count (>0 but != packet_length)
TEST_F(ApiTest, PingWritePartial) {
    _forceWriteOverride = true;
    _forceWriteReturn = 5;  // ping packet is 10 bytes; 5 is partial
    dynamixel_result_t result = dynamixel_send_ping(0x01, &_bus);
    ASSERT_EQ(result, DNM_LL_ERR);
}

// dynamixel_write: write timeout
TEST_F(ApiTest, WriteWriteTimeout) {
    _forceWriteOverride = true;
    _forceWriteReturn = 0;
    dynamixel_result_t result = dynamixel_write(0x01, 132, 1, 0xAA, &_bus);
    ASSERT_EQ(result, DNM_LL_TIMEOUT);
}

// dynamixel_write: write returns a partial count (>0 but != packet_length).
TEST_F(ApiTest, WriteWritePartial) {
    _forceWriteOverride = true;
    _forceWriteReturn = 5;
    dynamixel_result_t result = dynamixel_write(0x01, 132, 1, 0xAA, &_bus);
    ASSERT_EQ(result, DNM_LL_ERR);
}

TEST_F(ApiTest, WriteWriteError) {
    _writesize = -1;
    dynamixel_result_t result = dynamixel_write(0x01, 132, 1, 0xAA, &_bus);
    ASSERT_EQ(result, DNM_LL_ERR);
}

// dynamixel_write: read timeout (write succeeded, no response prepared)
TEST_F(ApiTest, WriteReadTimeout) {
    dynamixel_result_t result = dynamixel_write(0x01, 132, 1, 0xAA, &_bus);
    ASSERT_EQ(result, DNM_LL_TIMEOUT);
}

// dynamixel_write: read error
TEST_F(ApiTest, WriteReadError) {
    _readsize = -1;
    dynamixel_result_t result = dynamixel_write(0x01, 132, 1, 0xAA, &_bus);
    ASSERT_EQ(result, DNM_LL_ERR);
}

// dynamixel_read: write timeout
TEST_F(ApiTest, ReadWriteTimeout) {
    _forceWriteOverride = true;
    _forceWriteReturn = 0;
    uint32_t value;
    dynamixel_result_t result = dynamixel_read(0x01, 132, 1, &value, &_bus);
    ASSERT_EQ(result, DNM_LL_TIMEOUT);
}

// dynamixel_read: write partial
TEST_F(ApiTest, ReadWritePartial) {
    _forceWriteOverride = true;
    _forceWriteReturn = 5;
    uint32_t value;
    dynamixel_result_t result = dynamixel_read(0x01, 132, 1, &value, &_bus);
    ASSERT_EQ(result, DNM_LL_ERR);
}

// dynamixel_read: read timeout
TEST_F(ApiTest, ReadReadTimeout) {
    uint32_t value;
    dynamixel_result_t result = dynamixel_read(0x01, 132, 1, &value, &_bus);
    ASSERT_EQ(result, DNM_LL_TIMEOUT);
}

// dynamixel_read: read error
TEST_F(ApiTest, ReadReadError) {
    _readsize = -1;
    uint32_t value;
    dynamixel_result_t result = dynamixel_read(0x01, 132, 1, &value, &_bus);
    ASSERT_EQ(result, DNM_LL_ERR);
}

// dynamixel_sync_write: write timeout
TEST_F(ApiTest, SyncWriteWriteTimeout) {
    _forceWriteOverride = true;
    _forceWriteReturn = 0;
    uint8_t identifiers[] = {0x01};
    uint32_t values[] = {0x10};
    dynamixel_result_t result = dynamixel_sync_write(identifiers, 1, 116, 4, values, &_bus);
    ASSERT_EQ(result, DNM_LL_TIMEOUT);
}

// dynamixel_sync_write: write partial
TEST_F(ApiTest, SyncWriteWritePartial) {
    _forceWriteOverride = true;
    _forceWriteReturn = 5;
    uint8_t identifiers[] = {0x01};
    uint32_t values[] = {0x10};
    dynamixel_result_t result = dynamixel_sync_write(identifiers, 1, 116, 4, values, &_bus);
    ASSERT_EQ(result, DNM_LL_ERR);
}

// dynamixel_sync_read: write timeout
TEST_F(ApiTest, SyncReadWriteTimeout) {
    _forceWriteOverride = true;
    _forceWriteReturn = 0;
    uint8_t identifiers[] = {0x01};
    uint32_t values[1];
    dynamixel_result_t result = dynamixel_sync_read(identifiers, 1, 132, 4, values, &_bus);
    ASSERT_EQ(result, DNM_LL_TIMEOUT);
}

// dynamixel_sync_read: write partial
TEST_F(ApiTest, SyncReadWritePartial) {
    _forceWriteOverride = true;
    _forceWriteReturn = 5;
    uint8_t identifiers[] = {0x01};
    uint32_t values[1];
    dynamixel_result_t result = dynamixel_sync_read(identifiers, 1, 132, 4, values, &_bus);
    ASSERT_EQ(result, DNM_LL_ERR);
}

// dynamixel_sync_read: read timeout (no response prepared for any servo)
TEST_F(ApiTest, SyncReadReadTimeout) {
    uint8_t identifiers[] = {0x01};
    uint32_t values[1];
    dynamixel_result_t result = dynamixel_sync_read(identifiers, 1, 132, 4, values, &_bus);
    ASSERT_EQ(result, DNM_LL_TIMEOUT);
}

// dynamixel_sync_read: read error
TEST_F(ApiTest, SyncReadReadError) {
    _readsize = -1;
    uint8_t identifiers[] = {0x01};
    uint32_t values[1];
    dynamixel_result_t result = dynamixel_sync_read(identifiers, 1, 132, 4, values, &_bus);
    ASSERT_EQ(result, DNM_LL_ERR);
}

// dynamixel_read_response: bus_read returns 0 part-way through the payload.
// Header is delivered intact and claims payload_length=4, but the bus returns 0
// when asked for the payload bytes -> read_response returns 0 -> DNM_LL_TIMEOUT.
TEST_F(ApiTest, ResponsePayloadReadTimeout) {
    uint8_t header_only[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x04, 0x00};
    prepare_response(header_only, sizeof(header_only));

    dynamixel_result_t result = dynamixel_send_ping(0x01, &_bus);
    ASSERT_EQ(result, DNM_LL_TIMEOUT);
}

// dynamixel_read_response: defensive guard against a bus that returns more
// bytes than requested during the header read. The pre-subtraction check
// rejects with -1 -> wrapper returns DNM_LL_ERR. The over-read magnitude is
// chosen large enough that the original (broken) post-subtraction check would
// have silently passed, demonstrating the fix.
TEST_F(ApiTest, ResponseHeaderOverRead) {
    _forceReadOverride = true;
    _forceReadReturn = 200;        // 7 requested, 200 returned
    _forceReadAfterCalls = 0;

    dynamixel_result_t result = dynamixel_send_ping(0x01, &_bus);
    ASSERT_EQ(result, DNM_LL_ERR);
}

// dynamixel_read_response: defensive guard for the payload phase.
TEST_F(ApiTest, ResponsePayloadOverRead) {
    // Header says payload_length = 4 (passes the pre-bound check).
    uint8_t header_only[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x04, 0x00};
    prepare_response(header_only, sizeof(header_only));

    _forceReadOverride = true;
    _forceReadReturn = 200;        // 4 requested, 200 returned
    _forceReadAfterCalls = 1;      // header read uses real data; payload over-reads

    dynamixel_result_t result = dynamixel_send_ping(0x01, &_bus);
    ASSERT_EQ(result, DNM_LL_ERR);
}

// Header claims payload length 0x0105 (261 bytes). The full 16-bit length must
// be honored, and the value must be rejected because it exceeds the receive
// buffer (preventing an overrun in dynamixel_bus_read).
TEST_F(ApiTest, ResponsePayloadLengthExceedsBuffer) {
    uint8_t header_only[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x05, 0x01, 0x55};
    prepare_response(header_only, sizeof(header_only));

    dynamixel_result_t result = dynamixel_send_ping(0x01, &_bus);

    ASSERT_EQ(result, DNM_LL_ERR);
}

// sync_write with entry_size=1 must serialize exactly one value byte per
// servo. Regression guard for the per-block writer.
TEST_F(ApiTest, SyncWriteEntrySizeOne) {
    uint8_t identifiers[] = {0x0A, 0x0B, 0x0C};
    // High bytes intentionally non-zero to confirm only the LSB is written.
    uint32_t values[] = {0xDEAD0011, 0xBEEF0022, 0xCAFE0033};

    dynamixel_result_t result = dynamixel_sync_write(identifiers, 3, 0x0064, 1, values, &_bus);

    ASSERT_EQ(result, DNM_OK);
    // HDR(4) + ID(1) + LEN(2) + INSTR(1) + addr(2) + entry_size(2)
    //   + 3 * (id + 1 value byte) + CRC(2) = 20 bytes
    ASSERT_EQ(_writesize, 20);
    ASSERT_EQ(_writeBuffer[5], 0x0D);  // length low: 3 + 4 + 3*2 = 13
    ASSERT_EQ(_writeBuffer[6], 0x00);
    ASSERT_EQ(_writeBuffer[7], 0x83);  // SYNC_WRITE
    ASSERT_EQ(_writeBuffer[8], 0x64);  // addr low
    ASSERT_EQ(_writeBuffer[9], 0x00);  // addr high
    ASSERT_EQ(_writeBuffer[10], 0x01); // entry_size low
    ASSERT_EQ(_writeBuffer[11], 0x00); // entry_size high
    ASSERT_EQ(_writeBuffer[12], 0x0A); ASSERT_EQ(_writeBuffer[13], 0x11);
    ASSERT_EQ(_writeBuffer[14], 0x0B); ASSERT_EQ(_writeBuffer[15], 0x22);
    ASSERT_EQ(_writeBuffer[16], 0x0C); ASSERT_EQ(_writeBuffer[17], 0x33);
}

// sync_read with entry_size=1 should pull a single byte out of the response payload.
TEST_F(ApiTest, SyncReadEntrySizeOne) {
    // Single status response with 1-byte payload (value = 0xA6). Reused from ReadByte.
    uint8_t response[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x05, 0x00, 0x55,
        0x00, 0xA6, 0x87, 0x22
    };
    prepare_response(response, sizeof(response));

    uint8_t identifiers[] = {0x01};
    uint32_t values[1] = {0};
    dynamixel_result_t result = dynamixel_sync_read(identifiers, 1, 132, 1, values, &_bus);

    ASSERT_EQ(result, DNM_OK);
    ASSERT_EQ(values[0], 0xA6u);
}

// sync_read with entry_size=2 should pull two little-endian bytes out of the payload.
TEST_F(ApiTest, SyncReadEntrySizeTwo) {
    // Single status response with 2-byte payload (value = 0x02A6). Reused from ReadWord.
    uint8_t response[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x06, 0x00, 0x55,
        0x00, 0xA6, 0x02, 0xC3, 0x8F
    };
    prepare_response(response, sizeof(response));

    uint8_t identifiers[] = {0x01};
    uint32_t values[1] = {0};
    dynamixel_result_t result = dynamixel_sync_read(identifiers, 1, 132, 2, values, &_bus);

    ASSERT_EQ(result, DNM_OK);
    ASSERT_EQ(values[0], 0x02A6u);
}

// sync_write with entry_size=2 must serialize exactly two value bytes per servo.
TEST_F(ApiTest, SyncWriteEntrySizeTwo) {
    uint8_t identifiers[] = {0x01, 0x02};
    uint32_t values[] = {0xDEAD1234, 0xBEEF5678};

    dynamixel_result_t result = dynamixel_sync_write(identifiers, 2, 0x0064, 2, values, &_bus);

    ASSERT_EQ(result, DNM_OK);
    // HDR(4) + ID(1) + LEN(2) + INSTR(1) + addr(2) + entry_size(2)
    //   + 2 * (id + 2 value bytes) + CRC(2) = 20 bytes
    ASSERT_EQ(_writesize, 20);
    ASSERT_EQ(_writeBuffer[5], 0x0D);  // length low: 3 + 4 + 2*3 = 13
    ASSERT_EQ(_writeBuffer[10], 0x02); // entry_size low
    ASSERT_EQ(_writeBuffer[12], 0x01);
    ASSERT_EQ(_writeBuffer[13], 0x34); ASSERT_EQ(_writeBuffer[14], 0x12);
    ASSERT_EQ(_writeBuffer[15], 0x02);
    ASSERT_EQ(_writeBuffer[16], 0x78); ASSERT_EQ(_writeBuffer[17], 0x56);
}
