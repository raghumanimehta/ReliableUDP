#include "packet.hpp"
#include <cstring>
#include <arpa/inet.h>  // For htonl, htons functions
#include "logger.cpp"


using namespace std;


unique_ptr<packet> makePacket(vector<char>& packetData, uint32_t seqNo, uint8_t flag) 
{
    uint16_t payloadLen = packetData.size();
    if (payloadLen > MAX_PAYLOAD_SIZE) {
        LOG_ERROR("[MAKE-PACKET] Payload size exceeds maximum. Size: " + std::to_string(payloadLen) + " | Max: " + std::to_string(MAX_PAYLOAD_SIZE));
        return nullptr;
    }

    auto out = make_unique<packet>();
    
    out->seqNo = seqNo;
    out->payloadLen = payloadLen;
    out->flag = flag;
    memset(&(out->payload), 0, MAX_PAYLOAD_SIZE);
    
    char* buffer = packetData.data();
    std::memcpy(&(out->payload), buffer, payloadLen);
    
    LOG_INFO("[MAKE-PACKET] Packet created successfully. SeqNo: " + std::to_string(seqNo) + " | Payload size: " + std::to_string(payloadLen) + " | Flag: " + std::to_string(flag));
    return out; 
}

unique_ptr<packet> makeEmptyPacket() 
{
    auto out = make_unique<packet>(); 
    out->seqNo = 0;
    out->payloadLen = 0;
    out->flag = 0;
    memset(&(out->payload), 0, MAX_PAYLOAD_SIZE);
    LOG_INFO("[MAKE-EMPTY-PACKET] Empty packet created. SeqNo: 0 | Payload size: 0 | Flag: 0");
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
        LOG_ERROR("[SEND-PACKET] Failed to send packet. SeqNo: " + std::to_string(pkt.seqNo) + " | Payload size: " + std::to_string(pkt.payloadLen) + " | Error: " + std::string(strerror(errno)));
        return false;
    }
    LOG_INFO("[SEND-PACKET] Packet sent successfully. SeqNo: " + std::to_string(pkt.seqNo) + " | Payload size: " + std::to_string(pkt.payloadLen) + " | Total bytes sent: " + std::to_string(sentBytes));
    return true;
}

unique_ptr<packet> readPkt(int socketFd, struct sockaddr_in& origin) 
{
    char buf[MAX_PACKET_SIZE];
    socklen_t addrLen = sizeof(origin);
    ssize_t recvBytes = recvfrom(socketFd, buf, MAX_PACKET_SIZE, 0,
                                (struct sockaddr*)&(origin), &addrLen);

    if (recvBytes < 0) 
    {
        LOG_ERROR("[READ-PACKET] Failed to receive packet. Error: " + std::string(strerror(errno)));
        return nullptr; 
    }

    LOG_INFO("[READ-PACKET] Packet received successfully. Total bytes received: " + std::to_string(recvBytes));
    vector<char> data(buf, buf + recvBytes);
    auto pkt = deserializePacket(data);
    if (pkt != nullptr) {
        LOG_INFO("[READ-PACKET] Packet deserialized successfully. SeqNo: " + std::to_string(pkt->seqNo) + " | Payload size: " + std::to_string(pkt->payloadLen) + " | Flag: " + std::to_string(pkt->flag));
    }
    return pkt;
}
unique_ptr<packet> deserializePacket(vector<char>& dataBuffer) 
{
    if (dataBuffer.empty()) {
        LOG_ERROR("[DESERIALIZE-PACKET] Data buffer is empty");
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
        LOG_ERROR("[DESERIALIZE-PACKET] Payload size exceeds maximum. Size: " + std::to_string(out->payloadLen) + " | Max: " + std::to_string(MAX_PAYLOAD_SIZE));
        return nullptr;
    }

    if (offset + out->payloadLen <= dataBuffer.size()) {
        std::memcpy(out->payload, buffer + offset, out->payloadLen);
        LOG_INFO("[DESERIALIZE-PACKET] Packet deserialized successfully. SeqNo: " + std::to_string(out->seqNo) + " | Payload size: " + std::to_string(out->payloadLen) + " | Flag: " + std::to_string(flag));
    } else {
        LOG_ERROR("[DESERIALIZE-PACKET] Buffer size mismatch. Offset: " + std::to_string(offset) + " | Payload len: " + std::to_string(out->payloadLen) + " | Total buffer size: " + std::to_string(dataBuffer.size()));
        return nullptr;
    }

    return out;
}
