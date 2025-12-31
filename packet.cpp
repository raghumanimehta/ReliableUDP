#include "packet.hpp"
#include <cstring>
#include <arpa/inet.h>  // For htonl, htons functions


using namespace std;


unique_ptr<packet> makePacket(vector<char>& packetData, uint32_t seqNo, uint8_t flag) 
{
    uint16_t payloadLen = packetData.size();
    if (payloadLen > MAX_PAYLOAD_SIZE) return nullptr;

    auto out = make_unique<packet>();
    
    out->seqNo = seqNo;
    out->payloadLen = payloadLen;
    out->flag = flag;
    memset(&(out->payload), 0, MAX_PAYLOAD_SIZE);
    
    char* buffer = packetData.data();
    std::memcpy(&(out->payload), buffer, payloadLen);
    
    return out; 
}

unique_ptr<packet> makeEmptyPacket() 
{
    auto out = make_unique<packet>(); 
    out->seqNo = 0;
    out->payloadLen = 0;
    out->flag = 0;
    memset(&(out->payload), 0, MAX_PAYLOAD_SIZE);
    return out; 
}

vector<char> serializePacket(const struct packet& pkt) {
    size_t totalSize = HEADER_SIZE + pkt.payloadLen;
    std::vector<char> result(totalSize);
    
    char* buffer = result.data();
    size_t offset = 0;

    uint32_t netSeqNo = htonl(pkt.seqNo);
    std::memcpy(buffer + offset, &netSeqNo, sizeof(netSeqNo));
    offset += sizeof(netSeqNo);

    uint16_t netPayloadLen = htons(pkt.payloadLen); 
    std::memcpy(buffer + offset, &netPayloadLen, sizeof(netPayloadLen));
    offset += sizeof(netPayloadLen);

    std::memcpy(buffer + offset, &pkt.flag, sizeof(uint8_t));   
    offset += sizeof(uint8_t);

    if (pkt.payloadLen > 0) {
        std::memcpy(buffer + offset, pkt.payload, pkt.payloadLen);
    }

    return result;
}

#include <sys/socket.h>
#include <cerrno>
#include <string>
#include "logger.cpp"

bool sendPacket(int socketFd, const struct sockaddr_in& dst, const packet& pkt) {
    std::vector<char> serializedPkt = serializePacket(pkt);
    ssize_t sentBytes = sendto(socketFd, serializedPkt.data(), serializedPkt.size(), 0,
                        (const struct sockaddr*)&dst, sizeof(dst));
    if (sentBytes == -1) {
        LOG_ERROR("Failed to send packet: " + std::string(strerror(errno)));
        return false;
    }
    return true;
}

unique_ptr<packet> readPkt(int socketFd, struct sockaddr_in origin) 
{
    char buf[MAX_PACKET_SIZE];
    socklen_t addrLen = sizeof(origin);
    ssize_t recvBytes = recvfrom(socketFd, buf, MAX_PACKET_SIZE, 0,
                                (struct sockaddr*)&(origin), &addrLen);

    if (recvBytes < 0) 
    {
        LOG_ERROR("Failed to receive first packet: " + std::string(strerror(errno)));
        return nullptr; 
    }

    vector<char> data(buf, buf + recvBytes);
    auto pkt = deserializePacket(data);
    return pkt;
}
unique_ptr<packet> deserializePacket(vector<char>& dataBuffer) 
{
    if (dataBuffer.empty()) {
        return nullptr;
    }

    auto out = make_unique<packet>();
    size_t offset = 0;

    const char * buffer = dataBuffer.data();

    uint32_t netSeqNo;
    std::memcpy(&netSeqNo, buffer + offset, sizeof(uint32_t));
    out->seqNo = ntohl(netSeqNo);
    offset += sizeof(uint32_t);
    
    uint16_t netPayloadLen;
    std::memcpy(&netPayloadLen, buffer + offset, sizeof(uint16_t));
    out->payloadLen = ntohs(netPayloadLen);  
    offset += sizeof(uint16_t);

    uint8_t flag;
    std::memcpy(&flag, buffer + offset, sizeof(uint8_t));
    out->flag = flag;
    offset += sizeof(uint8_t);

    if (out->payloadLen > MAX_PAYLOAD_SIZE) {
        return nullptr;
    }

    if (offset + out->payloadLen <= dataBuffer.size()) {
        std::memcpy(out->payload, buffer + offset, out->payloadLen);
    } else {
        return nullptr;
    }

    return out;
}
