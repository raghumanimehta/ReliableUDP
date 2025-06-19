#include "packet.hpp"
#include <arpa/inet.h>  // For htonl, htons functions


using namespace std;


uint16_t computeChecksum(struct packet* pkt) 
{
    return 1;
}

vector<char> serializePacket(struct packet* pkt) {
    std::vector<char> result;

    size_t headerSize = sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t) + 2 * sizeof(uint8_t);



    char* buffer = result.data();
    size_t offset = 0;

    // Convert and copy each field
    uint32_t netSeqNo = htonl(pkt->seqNo);
    memcpy(buffer + offset, &netSeqNo, sizeof(netSeqNo));
    offset += sizeof(netSeqNo);

    uint16_t netChecksum = htons(pkt->checksum);
    memcpy(buffer + offset, &netChecksum, sizeof(netChecksum));
    offset += sizeof(netChecksum);

    uint16_t netPayloadLen = htons(pkt->payloadLen); 
    memcpy(buffer + offset, &netPayloadLen, sizeof(netPayloadLen));
    offset += sizeof(netPayloadLen);

    memcpy(buffer + offset, &pkt->isLast, sizeof(uint8_t));   
    offset += sizeof(uint8_t);

    memcpy(buffer + offset, &pkt->flag, sizeof(uint8_t));   
    offset += sizeof(uint8_t);

    if (pkt->payloadLen > 0 && pkt->payload) {
        memcpy(buffer + offset, pkt->payload, pkt->payloadLen);
    }

    return result;
}

struct packet * deserializePacket(vector<char>& dataBuffer) 
{
    if (dataBuffer.empty()) {
         return nullptr;
    }

    struct packet * out = new struct packet;
    size_t offset = 0;

    const char * buffer = dataBuffer.data();

    uint32_t netSeqNo;
    memcpy(&netSeqNo, buffer + offset, sizeof(uint32_t));
    out->seqNo = ntohl(netSeqNo);
    offset += sizeof(uint32_t);
    
    uint16_t netChecksum;
    memcpy(&netChecksum, buffer + offset, sizeof(uint16_t));
    out->checksum = ntohs(netChecksum); 
    offset += sizeof(uint16_t);

    uint16_t netPayloadLen;
    memcpy(&netPayloadLen, buffer + offset, sizeof(uint16_t));
    out->payloadLen = ntohs(netPayloadLen);  
    offset += sizeof(uint16_t);

    uint8_t isLast;
    memcpy(&isLast, buffer + offset, sizeof(uint8_t));
    out->isLast = isLast;
    offset += sizeof(uint8_t);

    uint8_t flag;
    memcpy(&flag, buffer + offset, sizeof(uint8_t));
    out->flag = flag;
    offset += sizeof(uint8_t);

    if (out->payloadLen > MAX_PAYLOAD_SIZE) {
        delete out;
        return nullptr;
    }

    if (offset + out->payloadLen <= dataBuffer.size()) {
        memcpy(out->payload, buffer + offset, out->payloadLen);
    } else {
        delete out;
        return nullptr;
    }

    return out;
}