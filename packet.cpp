#include "packet.hpp"
#include <cstring>
#include <arpa/inet.h>  // For htonl, htons functions


using namespace std;


unique_ptr<packet> makePacket(vector<char>& packetData) 
{
    uint16_t payloadLen = packetData.size();
    if (payloadLen > MAX_PAYLOAD_SIZE) return nullptr;

    auto out = make_unique<packet>();
    
    out->seqNo = 0;
    out->payloadLen = payloadLen;
    out->flag = 0;
    memset(&(out->payload), 0, MAX_PAYLOAD_SIZE);
    
    char* buffer = packetData.data();
    std::memcpy(&(out->payload), buffer, payloadLen);
    
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