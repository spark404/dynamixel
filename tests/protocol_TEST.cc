//
// Created by Hugo Trippaers on 18/04/2025.
//
// Demonstrate some basic assertions.

#include <gtest/gtest.h>

#include "../priv_include/dynamixel/protocol.h"

TEST(ProtocolTests, HeaderDefaults) {
    uint8_t expected[] = {0xFF, 0xFF, 0xFD, 0x00, 0x00, 0x00, 0x00, 0x00};
    dynamixel_packet_header_t header = DYNAMIXEL_PACKET_HEADER_DEFAULT;

    ASSERT_EQ(sizeof(dynamixel_packet_header_t), 8);
    ASSERT_EQ(memcmp(&header, expected, 8), 0);
}

TEST(ProtocolTests, PingHeader) {
    dynamixel_packet_header_t header = DYNAMIXEL_PACKET_HEADER_DEFAULT;
    header.id = 0x1;
    header.instruction = PING;
    header.length = 3;

    uint8_t expected[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x03, 0x00, 0x01};

    ASSERT_EQ(sizeof(dynamixel_packet_header_t), 8);
    ASSERT_EQ(memcmp(&header, expected, 8), 0);
}

TEST(ProtocolTests, StatusHeader) {
    uint8_t received[] = {0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x07, 0x00, 0x55, 0x03};

    auto *packet = reinterpret_cast<dynamixel_status_packet_header_t *>(received);

    ASSERT_EQ(packet->header[0], 0xFF);
    ASSERT_EQ(packet->header[1], 0xFF);
    ASSERT_EQ(packet->header[2], 0xFD);
    ASSERT_EQ(packet->reserved, 0x0);
    ASSERT_EQ(packet->id, 0x01);
    ASSERT_EQ(packet->length, 7);
    ASSERT_EQ(packet->instruction, STATUS);
    ASSERT_EQ(packet->error, STATUS_CRC_ERROR);
}

TEST(ProtocolTests, PingPacket) {
    dynamixel_packet_header_t header = DYNAMIXEL_PACKET_HEADER_DEFAULT;
    header.id = 0xFE;
    header.instruction = PING;
    header.length = 3;

    uint8_t buffer[256];
    size_t packet_length;

    uint8_t expected[] = {
        0xFF, 0xFF, 0xFD, 0x00,
        0xFE, 0x03, 0x00, 0x01,
        0x31, 0x42
    };

    const dynamixel_error_t result = dynamixel_build_packet(header, nullptr, 0, buffer, sizeof(buffer), &packet_length);

    ASSERT_EQ(result, DYNAMIXEL_ERROR_NONE);
    ASSERT_EQ(packet_length, sizeof(expected));
    ASSERT_EQ(memcmp(buffer, expected, packet_length), 0);
}

TEST(ProtocolTests, ReadPacket) {
    dynamixel_packet_header_t header = DYNAMIXEL_PACKET_HEADER_DEFAULT;
    header.id = 0x1;
    header.instruction = READ;
    header.length = 7;

    uint8_t param[4] = {0x84, 0x00, 0x04, 0x00};

    uint8_t buffer[256];
    size_t packet_length;

    const uint8_t expected[] = {
        0xFF, 0xFF, 0xFD, 0x00,
        0x01, 0x07, 0x00, 0x02,
        0x84, 0x00, 0x04, 0x00,
        0x1D, 0x15
    };

    const dynamixel_error_t result = dynamixel_build_packet(header, param, sizeof(param), buffer, sizeof(buffer),
                                                            &packet_length);

    ASSERT_EQ(result, DYNAMIXEL_ERROR_NONE);
    ASSERT_EQ(packet_length, sizeof(expected));
    ASSERT_EQ(memcmp(buffer, expected, packet_length), 0);
}

TEST(ProtocolTests, InvalidHeaderLength) {
    dynamixel_packet_header_t header = DYNAMIXEL_PACKET_HEADER_DEFAULT;
    header.id = 0x1;
    header.instruction = READ;
    header.length = 88;

    uint8_t param[4] = {0x84, 0x00, 0x04, 0x00};

    uint8_t buffer[256];
    size_t packet_length;

    const dynamixel_error_t result = dynamixel_build_packet(header, param, sizeof(param), buffer, sizeof(buffer),
                                                            &packet_length);

    ASSERT_EQ(result, DYNAMIXEL_ERROR_LENGTH_MISMATCH);
}

TEST(ProtocolTests, BufferTooSmall) {
    dynamixel_packet_header_t header = DYNAMIXEL_PACKET_HEADER_DEFAULT;
    header.id = 0x1;
    header.instruction = READ;
    header.length = 88;

    uint8_t param[4] = {0x84, 0x00, 0x04, 0x00};

    uint8_t buffer[11];
    size_t packet_length;

    const dynamixel_error_t result = dynamixel_build_packet(header, param, sizeof(param), buffer, sizeof(buffer),
                                                            &packet_length);

    ASSERT_EQ(result, DYNAMIXEL_ERROR_BUFFER_TOO_SMALL);
}
