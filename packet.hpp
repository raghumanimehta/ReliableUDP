#ifndef PACKET_HPP
#define PACKET_HPP

#include <cstdint>
#include <vector> 
#include <cassert>
#include <memory> 

const uint32_t MAX_PAYLOAD_SIZE = 1024; 
const uint32_t HEADER_SIZE  = 7;        // size of deserialized struct excluding the payload
const uint32_t MAX_PACKET_SIZE = MAX_PAYLOAD_SIZE + HEADER_SIZE;

constexpr uint8_t FLAG_DATA = 0x00;
constexpr uint8_t FLAG_ACK  = 0x01;
constexpr uint8_t FLAG_SYN  = 0x02;
constexpr uint8_t FLAG_FIN  = 0x04;
constexpr uint8_t FLAG_SYN_ACK  = FLAG_SYN | FLAG_ACK;

struct packet {
    uint32_t    seqNo; 
    uint16_t    payloadLen;
    uint8_t     flag;                   
    char payload[MAX_PAYLOAD_SIZE];
};

std::unique_ptr<packet> makePacket(std::vector<char>& packetData, uint32_t seqNo, uint8_t flag);
std::unique_ptr<packet> makeEmptyPacket();
std::vector<char> serializePacket(const struct packet& pkt);
std::unique_ptr<packet> deserializePacket(std::vector<char>& dataBuffer);

#endif 
                            