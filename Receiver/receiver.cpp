#include "receiver.hpp"

// Required for socket functions
#include <sys/socket.h>  

// Required for address structures
#include <netinet/in.h>  

// Required for inet_addr and related functions
#include <arpa/inet.h>   // For inet_addr(), inet_ntoa(), etc.

// Required for close()
#include <unistd.h>     

#include <iostream>    
#include <string>
#include <cstring>    
#include <poll.h>
#include <fstream>
#include <stdio.h>
#include "../packet.hpp"
#include "../logger.cpp"
#include "../utils.hpp"

using namespace std;

static std::string receiverStateToString(ReceieverState s)
{
    switch (s)
    {
    case ReceieverState::IDLE:
        return "IDLE";
    case ReceieverState::SYN_RECV:
        return "SYN_RECV";
    case ReceieverState::ACK_SENT:
        return "ACK_SENT";
    case ReceieverState::CONNECTED:
        return "CONNECTED";
    case ReceieverState::CLOSING_INIT:
        return "CLOSING_INIT";
    case ReceieverState::CLOSING:
        return "CLOSING";
    case ReceieverState::CLOSED:
        return "CLOSED";
    default:
        return "UNKNOWN";
    }
}

Receiver::Receiver()
    : socketFd(-1) 
{
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd == -1) {
        LOG_ERROR("[RECEIVER-INIT] Failed to create UDP socket. Error: " + std::string(strerror(errno)));
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);        
    addr.sin_addr.s_addr = INADDR_ANY;  
    if (::bind(socketFd, (struct sockaddr *)&addr, sizeof(addr)) ==  -1) {
        LOG_ERROR("[RECEIVER-INIT] Bind failed on port " + std::to_string(PORT) + ". Error: " + std::string(strerror(errno)));
        throw std::runtime_error("bind failed: " + std::string(strerror(errno)));
    }
    origin = {};
    state = ReceieverState::IDLE;
    LOG_INFO("[RECEIVER-INIT] Successfully initialized. State: IDLE | Socket FD: " + std::to_string(socketFd) + " | Bound to port: " + std::to_string(PORT));
}

Receiver::~Receiver() 
{
    if (socketFd >= 0) {
        close(socketFd); 
    }
}


bool Receiver::receiveFile()
{
    LOG_INFO("[RECEIVE-FILE] Starting file reception.\n"
             "  State: " + receiverStateToString(this->state));
    if (!handshake()) {
        LOG_ERROR("[RECEIVE-FILE] Handshake failed. State: " + receiverStateToString(this->state));
        return false;
    }
    const uint64_t TIMEOUT_MS = 1000;
    const uint32_t RETRIES = 1000;

    LOG_INFO("[RECEIVE-FILE] Handshake completed.\n"
             "  State: " + receiverStateToString(this->state) +
             "\n  Waiting for data packets");
    if (waitForReadWithRetry(socketFd, TIMEOUT_MS, RETRIES) != SUCCESS)
    {
        LOG_ERROR("[RECEIVE-FILE] Timeout or error waiting for data packet. State: " + receiverStateToString(this->state));
        return false; 
    }

    auto pkt = readPkt(socketFd, origin);
    if (pkt == nullptr) 
    {
        LOG_ERROR("[RECEIVE-FILE] Failed to read data packet. State: " + receiverStateToString(this->state));
        return false;
    }
    char recvPayload[MAX_PAYLOAD_SIZE];
    memcpy(recvPayload, pkt->payload, pkt->payloadLen);
    // TODO: Handle the checks on the packet here
    LOG_INFO("[RECEIVE-FILE] Packet received successfully.\n"
             "  State: " + receiverStateToString(this->state) +
             "\n  SeqNo: " + std::to_string(pkt->seqNo) +
             "\n  Payload size: " + std::to_string(pkt->payloadLen) + " bytes"
             "\n  Flag: " + std::string(pkt->flag == FLAG_FIN ? "FIN" : "DATA"));

    // Write the raw received data to output.txt to verify UDP transmission
    std::ofstream outFile("output.txt", std::ios::binary);
    if (outFile.is_open())
    {    
        outFile.write(recvPayload, pkt->payloadLen);
        outFile.close();
        LOG_INFO("[RECEIVE-FILE] File data written successfully.\n"
             "  Output file: output.txt\n"
             "  Bytes written: " + std::to_string(pkt->payloadLen));
    }
    else
    {
        LOG_ERROR("[RECEIVE-FILE] Failed to open output.txt for writing");
        return false;
    }

    LOG_INFO("[RECEIVE-FILE] File reception completed successfully.\n"
             "  State: " + receiverStateToString(this->state));
    return true;
}

bool Receiver::waitAndUpdateState(uint64_t timeout, uint32_t retries, uint32_t expectedSeqNo,  uint8_t expectedFlag, ReceieverState nextState) 
{
    if (waitForReadWithRetry(socketFd, timeout, retries) != SUCCESS)
    {
        LOG_ERROR("[HANDSHAKE] Timeout waiting for SYN packet. State: " + receiverStateToString(this->state));
        return false;
    }

    auto pkt = readPkt(socketFd, origin);
    if (pkt == nullptr) 
    {
        LOG_ERROR("[HANDSHAKE] Failed to read SYN packet. State: " + receiverStateToString(this->state));
        return false;
    }

    if (pkt->flag == expectedFlag && pkt->seqNo == expectedSeqNo) 
    {
        this->state = nextState;
        return true;
    }  else {
        LOG_ERROR("[HANDSHAKE] Received packet with unexpected values. State: " + receiverStateToString(this->state) + " | SeqNo: " + std::to_string(pkt->seqNo) + " (Expected: " + std::to_string(SYN_SEQNO) + ") | Flag: " + std::to_string(pkt->flag) + " (Expected: SYN)");
        return false;
    }
}

bool Receiver::handshake() 
{
    LOG_INFO("[HANDSHAKE] Starting handshake.\n"
             "  State: " + receiverStateToString(this->state) +
             "\n  Waiting for SYN packet from sender");

    const uint64_t TIMEOUT_MS = 1000;
    const uint8_t RETRIES = 10;
    if (this->waitAndUpdateState(TIMEOUT_MS, RETRIES, SYN_SEQNO, FLAG_SYN, ReceieverState::SYN_RECV)) 
    {
        LOG_INFO("[HANDSHAKE] Received valid SYN packet.\n"
             "  State: " + receiverStateToString(this->state) + " -> SYN_RECV"
             "\n  SeqNo: " + std::to_string(SYN_SEQNO) +
             "\n  Flag: SYN");
    } else {
        return false;
    }
    auto sendPkt = makeEmptyPacket();
    sendPkt->flag = FLAG_SYN_ACK;
    sendPkt->seqNo = SYN_SEQNO + 1;
    LOG_INFO("[HANDSHAKE] Sending SYN-ACK response.\n"
             "  State: " + receiverStateToString(this->state) + " -> ACK_SENT"
             "\n  SeqNo: " + std::to_string(sendPkt->seqNo) +
             "\n  Flag: SYN_ACK");
    if (!sendPacket(this->socketFd, this->origin, *sendPkt)) {
        LOG_ERROR("[HANDSHAKE] Failed to send SYN-ACK packet. State: " + receiverStateToString(this->state));
        return false;
    }
    this->state = ReceieverState::ACK_SENT;

    if (this->waitAndUpdateState(TIMEOUT_MS, RETRIES, SYN_SEQNO + 1, FLAG_ACK, ReceieverState::CONNECTED)) 
    {
        LOG_INFO("[HANDSHAKE] Received valid SYN packet.\n"
             "  State: " + receiverStateToString(this->state) + " -> SYN_RECV"
             "\n  SeqNo: " + std::to_string(SYN_SEQNO + 1) +
             "\n  Flag: SYN");
    } else {
        return false;
    }
    LOG_INFO("[HANDSHAKE] Three-way handshake completed successfully.\n"
             "  State: " + receiverStateToString(this->state));
    return true;
}