#include <cstdint>

const uint32_t MAX_PAYLOAD_SIZE = 1024; 

struct packet {
    uint32_t seqNo; // Sequence number
    char payload[MAX_PAYLOAD_SIZE];
    uint16_t checksum;
    bool     isLast; // True when this is the last packet to send 
};

                            