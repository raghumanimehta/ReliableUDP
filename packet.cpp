#include "packet.hpp"
#include <arpa/inet.h>  // For htonl, htons functions


using namespace std;


void computeChecksum(vector<char>& serializedPkt) 
/* 
Computes and stores the checksum at the right offset in the serialized packet 
*/
{
    return;
}

struct packet* makePacket(vector<char>& packetData) 
// Function makes a packet and returns it
// This function checks the size of the data and returns null if the data size is more than max payload length
{
    uint16_t payloadLen = packetData.size();
    if (payloadLen > MAX_PAYLOAD_SIZE) return nullptr;

    struct packet* out = new struct packet();
    
    // Initialize all fields to default values
    out->seqNo = 0;
    out->checksum = 0;
    out->payloadLen = payloadLen;
    out->isLast = 0;
    out->flag = 0;
    // Zero out the payload buffer first
    memset(&(out->payload), 0, MAX_PAYLOAD_SIZE);
    
    // Copy the actual payload data
    char* buffer = packetData.data();
    memcpy(&(out->payload), buffer, payloadLen);
    
    return out; 
}

vector<char> serializePacket(struct packet* pkt) {
    // Calculate total size needed
    size_t headerSize = sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t) + 2 * sizeof(uint8_t);
    size_t totalSize = headerSize + pkt->payloadLen;
    
    // Allocate the right amount of space
    std::vector<char> result(totalSize);
    
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