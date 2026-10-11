//
// Created by Hugo Trippaers on 18/04/2025.
//
// Demonstrate some basic assertions.

#include <random>
#include <vector>
#include <cstring>
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

TEST(ProtocolTests, StuffedLength) {
    const uint8_t none[] = {0xFF, 0xFF, 0x00, 0xFD, 0xFF, 0xFD};
    EXPECT_EQ(dynamixel_stuffed_length(none, sizeof(none)), sizeof(none));
    const uint8_t one[] = {0x01, 0xFF, 0xFF, 0xFD, 0x02};
    EXPECT_EQ(dynamixel_stuffed_length(one, sizeof(one)), sizeof(one) + 1);
    const uint8_t two[] = {0xFF, 0xFF, 0xFD, 0xFF, 0xFF, 0xFD};
    EXPECT_EQ(dynamixel_stuffed_length(two, sizeof(two)), sizeof(two) + 2);
    const uint8_t overlap[] = {0xFF, 0xFF, 0xFF, 0xFD};
    EXPECT_EQ(dynamixel_stuffed_length(overlap, sizeof(overlap)), sizeof(overlap) + 1);
    const uint8_t stuffed_already[] = {0xFF, 0xFF, 0xFD, 0xFD};
    EXPECT_EQ(dynamixel_stuffed_length(stuffed_already, sizeof(stuffed_already)), sizeof(stuffed_already) + 1);
    EXPECT_EQ(dynamixel_stuffed_length(nullptr, 0), 0u);
}

TEST(ProtocolTests, UnstuffKnownCases) {
    struct Case { std::vector<uint8_t> stuffed; std::vector<uint8_t> raw; };
    const Case cases[] = {
        {{}, {}},
        {{0x01, 0x02}, {0x01, 0x02}},
        {{0xFF, 0xFF, 0xFD, 0xFD}, {0xFF, 0xFF, 0xFD}},
        {{0x05, 0xFF, 0xFF, 0xFD, 0xFD, 0x06}, {0x05, 0xFF, 0xFF, 0xFD, 0x06}},
        {{0xFF, 0xFF, 0xFF, 0xFD, 0xFD}, {0xFF, 0xFF, 0xFF, 0xFD}},
        {{0xFF, 0xFF, 0xFD, 0xFD, 0xFD}, {0xFF, 0xFF, 0xFD, 0xFD}},
        {{0xFF, 0xFF, 0xFD, 0xFD, 0xFF, 0xFF, 0xFD, 0xFD}, {0xFF, 0xFF, 0xFD, 0xFF, 0xFF, 0xFD}},
        {{0xFD, 0xFD, 0xFF, 0xFF}, {0xFD, 0xFD, 0xFF, 0xFF}},
    };
    for (const auto &c : cases) {
        std::vector<uint8_t> data = c.stuffed;
        size_t length = 99;
        ASSERT_EQ(dynamixel_unstuff(data.data(), data.size(), &length), DYNAMIXEL_ERROR_NONE);
        ASSERT_EQ(length, c.raw.size());
        EXPECT_EQ(std::vector<uint8_t>(data.begin(), data.begin() + length), c.raw);
    }
}

TEST(ProtocolTests, UnstuffRejectsMissingStuffedByte) {
    size_t length = 0;
    std::vector<uint8_t> at_end = {0x01, 0xFF, 0xFF, 0xFD};
    EXPECT_EQ(dynamixel_unstuff(at_end.data(), at_end.size(), &length), DYNAMIXEL_ERROR_INVALID_PACKET);
    std::vector<uint8_t> wrong_byte = {0xFF, 0xFF, 0xFD, 0x00, 0x01};
    EXPECT_EQ(dynamixel_unstuff(wrong_byte.data(), wrong_byte.size(), &length), DYNAMIXEL_ERROR_INVALID_PACKET);
}

TEST(ProtocolTests, StuffUnstuffRoundTrip) {
    // Parameters made of the bytes that matter most for stuffing, compared with an
    // independent reference implementation of the rule
    std::mt19937 rng(12345);
    const uint8_t alphabet[] = {0xFF, 0xFF, 0xFD, 0xFD, 0x00, 0x12};
    for (int round = 0; round < 20000; round++) {
        const size_t n = rng() % 40;
        std::vector<uint8_t> raw(n);
        for (auto &b : raw) b = alphabet[rng() % sizeof(alphabet)];

        std::vector<uint8_t> reference;
        for (size_t i = 0; i < n; i++) {
            reference.push_back(raw[i]);
            if (i >= 2 && raw[i] == 0xFD && raw[i - 1] == 0xFF && raw[i - 2] == 0xFF) reference.push_back(0xFD);
        }
        ASSERT_EQ(dynamixel_stuffed_length(raw.data(), n), reference.size());

        dynamixel_packet_header_t header = DYNAMIXEL_PACKET_HEADER_DEFAULT;
        header.id = 0x01;
        header.instruction = WRITE;
        header.length = n + 3;
        uint8_t buffer[256];
        size_t packet_length = 0;
        ASSERT_EQ(dynamixel_build_packet(header, raw.data(), n, buffer, sizeof(buffer), &packet_length),
                  DYNAMIXEL_ERROR_NONE);
        ASSERT_EQ(packet_length, 8 + reference.size() + 2);
        ASSERT_EQ(buffer[5] | (buffer[6] << 8), (int) reference.size() + 3) << "length field counts the stuffing";
        ASSERT_EQ(std::vector<uint8_t>(buffer + 8, buffer + 8 + reference.size()), reference);

        size_t unstuffed = 0;
        ASSERT_EQ(dynamixel_unstuff(buffer + 8, reference.size(), &unstuffed), DYNAMIXEL_ERROR_NONE);
        ASSERT_EQ(unstuffed, n);
        ASSERT_EQ(std::vector<uint8_t>(buffer + 8, buffer + 8 + n), raw);
    }
}

TEST(ProtocolTests, BuildPacketStuffsParameters) {
    // Write of 4 bytes FF FF FD 00 to address 116 of servo 1
    const uint8_t expected[] = {
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x0A, 0x00, 0x03, 0x74, 0x00, 0xFF, 0xFF,
        0xFD, 0xFD, 0x00, 0x21, 0xE7,
    };
    dynamixel_packet_header_t header = DYNAMIXEL_PACKET_HEADER_DEFAULT;
    header.id = 0x01;
    header.instruction = WRITE;
    header.length = 6 + 3;
    uint8_t param[] = {116, 0, 0xFF, 0xFF, 0xFD, 0x00};
    uint8_t buffer[64];
    size_t packet_length = 0;

    ASSERT_EQ(dynamixel_build_packet(header, param, sizeof(param), buffer, sizeof(buffer), &packet_length),
              DYNAMIXEL_ERROR_NONE);
    ASSERT_EQ(packet_length, sizeof(expected));
    EXPECT_EQ(memcmp(buffer, expected, sizeof(expected)), 0);
}

TEST(ProtocolTests, BuildPacketStuffedBufferTooSmall) {
    // The packet fits unstuffed (13 bytes) but not stuffed (14 bytes)
    dynamixel_packet_header_t header = DYNAMIXEL_PACKET_HEADER_DEFAULT;
    header.id = 0x01;
    header.instruction = WRITE;
    header.length = 3 + 3;
    uint8_t param[] = {0xFF, 0xFF, 0xFD};
    uint8_t buffer[13];
    size_t packet_length = 0;

    ASSERT_EQ(dynamixel_build_packet(header, param, sizeof(param), buffer, sizeof(buffer), &packet_length),
              DYNAMIXEL_ERROR_BUFFER_TOO_SMALL);
}
