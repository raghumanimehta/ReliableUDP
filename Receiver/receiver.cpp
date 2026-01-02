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

Receiver::Receiver()
    : socketFd(-1) 
{
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd == -1) {
        LOG_ERROR("Failed to create socket: " + std::string(strerror(errno)));
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);        
    addr.sin_addr.s_addr = INADDR_ANY;  
    if (::bind(socketFd, (struct sockaddr *)&addr, sizeof(addr)) ==  -1) {
        LOG_ERROR("bind failed: " + std::string(strerror(errno)));
        throw std::runtime_error("bind failed: " + std::string(strerror(errno)));
    }
    origin = {};
    state = ReceieverState::IDLE;
    LOG_INFO("Receiver created and bound to port: " + std::to_string(PORT));
}

Receiver::~Receiver() 
{
    if (socketFd >= 0) {
        close(socketFd); 
    }
}


bool Receiver::receiveFile()
{
   if (!handshake()) return false;      
   const uint64_t TIMEOUT_MS = 1000;
   const uint8_t RETRIES = 10;

   if (waitForReadWithRetry(socketFd, TIMEOUT_MS, RETRIES) != SUCCESS)
   {
    LOG_ERROR("Timeout or error waiting for SYN");
    return false; 
}

    auto pkt = readPkt(socketFd, origin);
    if (pkt == nullptr) 
    {
        LOG_INFO("Null packet received");
        return false;
    }
   char recvPayload[MAX_PAYLOAD_SIZE];
   memcpy(recvPayload, pkt->payload, pkt->payloadLen);
   // TODO: Handle the checks on the packet here
   
   // Write the raw received data to output.txt to verify UDP transmission
   std::ofstream outFile("output.txt", std::ios::binary);
   if (outFile.is_open())
   {    
        outFile.write(recvPayload, pkt->payloadLen);
       outFile.close();
       LOG_INFO("Successfully wrote received data to output.txt");
   }
   else
   {
       LOG_ERROR("Failed to open output.txt for writing");
   }

    return true;
}


bool Receiver::handshake() 
{
    LOG_INFO("State: IDLE, Waiting for connection (first packet)...");

    const uint64_t TIMEOUT_MS = 1000;
    const uint8_t RETRIES = 10;
    // ADD a loop here if the check for the FLAG and SEQ no fails
    if (waitForReadWithRetry(socketFd, TIMEOUT_MS, RETRIES) != SUCCESS)
    {
        LOG_ERROR("Timeout or error waiting for SYN");
        return false;
    }

    auto pkt = readPkt(socketFd, origin);
    if (pkt == nullptr) 
    {
        LOG_INFO("Null packet received");
        return false;
    }

    if (pkt->flag == FLAG_SYN && pkt->seqNo == SYN_SEQNO) 
    {
        LOG_INFO("SYN received");
        this->state = ReceieverState::SYN_RECV;
    }  else 
    {
        LOG_ERROR("The sequence number or flag did not match");
        return false;
    }

    auto sendPkt = makeEmptyPacket();
    sendPkt->flag = FLAG_SYN_ACK;
    sendPkt->seqNo = SYN_SEQNO + 1;
    if (!sendPacket(this->socketFd, this->origin, *sendPkt)) return false;
    this->state = ReceieverState::ACK_SENT;

    if (waitForReadWithRetry(socketFd, TIMEOUT_MS, RETRIES) != SUCCESS)
    {
        LOG_ERROR("Timeout or error waiting for SYN");
        return false;
    }

    auto pkt2 = readPkt(socketFd, origin);
    if (pkt2 == nullptr) 
    {
        LOG_INFO("Null packet received");
        return false;
    }

    if (pkt2->flag == FLAG_ACK && pkt2->seqNo == SYN_SEQNO + 1) 
    {
        LOG_INFO("SYN received");
        this->state = ReceieverState::SYN_RECV;
    }  else 
    {
        LOG_ERROR("The sequence number or flag did not match");
        return false;
    }
    this->state = ReceieverState::CONNECTED;
    return true;

}