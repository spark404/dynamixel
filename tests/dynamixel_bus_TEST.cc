//
// Created by Hugo Trippaers on 18/04/2025.
//
#include <gtest/gtest.h>

#include "dynamixel/dynamixel_bus.h"

ssize_t mock_write(const uint8_t *txBuffer, const size_t size, void *pvArgument) {
    auto *data_buffer = static_cast<uint8_t *>(pvArgument);
    memcpy(data_buffer + 1, txBuffer, size);
    return static_cast<uint8_t *>(pvArgument)[0];;
};

ssize_t mock_read(uint8_t *rxBuffer, const size_t size, void *pvArgument) {
    (void)size;
    auto *data_buffer = static_cast<uint8_t *>(pvArgument);
    memcpy(rxBuffer, data_buffer + 1, data_buffer[0]);
    return static_cast<uint8_t *>(pvArgument)[0];;
};

TEST(DynamixelBus, Init) {
    dynamixel_bus_t mock_bus;
    uint8_t buffer[128];
    buffer[0] = DNM_OK;

    auto result = dynamixel_bus_init(&mock_bus, mock_read, mock_write, buffer);

    ASSERT_EQ(mock_bus.pvContext, buffer);
    ASSERT_EQ(result, DNM_OK);
}

TEST(DynamixelBus, Read) {
    dynamixel_bus_t mock_bus;
    uint8_t buffer[128];
    buffer[0] = 3;

    dynamixel_bus_init(&mock_bus, mock_read, mock_write, buffer);

    buffer[1] = 0x01;
    buffer[2] = 0x02;
    buffer[3] = 0x03;

    uint8_t data[128];
    auto result = dynamixel_bus_read(&mock_bus, data, sizeof(data));

    ASSERT_EQ(mock_bus.pvContext, buffer);
    ASSERT_EQ(result, 3);
    ASSERT_EQ(data[0], 1);
    ASSERT_EQ(data[1], 2);
    ASSERT_EQ(data[2], 3);
}

TEST(DynamixelBus, Write) {
    dynamixel_bus_t mock_bus;
    uint8_t buffer[128];
    buffer[0] = 3;

    dynamixel_bus_init(&mock_bus, mock_read, mock_write, buffer);

    uint8_t data[] = { 0x01, 0x02, 0x03};
    auto result = dynamixel_bus_write(&mock_bus, data, sizeof(data));

    ASSERT_EQ(mock_bus.pvContext, buffer);
    ASSERT_EQ(result, sizeof(data));
    ASSERT_EQ(buffer[1], 1);
    ASSERT_EQ(buffer[2], 2);
    ASSERT_EQ(buffer[3], 3);
}