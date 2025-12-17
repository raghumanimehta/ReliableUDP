#ifndef PACKET_HPP
#define PACKET_HPP

#include <cstdint>
#include <vector> 
#include <cassert>
#include <memory> 

const uint32_t MAX_PAYLOAD_SIZE = 1024; 

struct packet {
    uint32_t seqNo; 
    uint16_t    payloadLen;
    uint8_t     isLast; // 1 for last, 0 otherwise
    uint8_t     flag; // 1 for Ack, 0 otherwise  
    char payload[MAX_PAYLOAD_SIZE];
};

std::unique_ptr<packet> makePacket(std::vector<char>& packetData);
std::vector<char> serializePacket(const struct packet& pkt);
std::unique_ptr<packet> deserializePacket(std::vector<char>& dataBuffer);

#endif // PACKET_HPP
                            