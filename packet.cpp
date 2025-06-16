#include "packet.hpp"
#include <arpa/inet.h>  // For htonl, htons functions


using namespace std;


uint16_t computeChecksum(struct packet* pkt) 
{
    return 1;
}


// struct packet* makePacket(vector<char>& data, uint32_t seqNo, bool isLast)  
// {
//     struct packet* pkt = new struct packet;
//     pkt->seqNo = seqNo;
//     pkt->
//     pkt->checksum = computeChecksum(pkt);
//     return pkt;
// }


std::vector<char> serializePacket(struct packet* pkt) {
    std::vector<char> result;

    size_t headerSize = sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t) + 2 * sizeof(uint8_t);
    size_t totalSize = headerSize + pkt->payloadLen;

    result.resize(totalSize);
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
