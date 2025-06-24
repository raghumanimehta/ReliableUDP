#include "sender.hpp"

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
#include "../packet.hpp"
#include "../logger.cpp"

using namespace std;

Sender::Sender(const std::string& destIp, const int port)
    : destinationIP(destIp), destinationPort(port), socketFd(-1) 
{
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd == -1) {
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }   
}

Sender::~Sender() 
{
    if (socketFd >= 0) {
        close(socketFd); 
    }
}

bool Sender::sendFile(const std::vector<char>& fileData) 
/*
Sends the file over UDP connection. 
Currently basic version only. Keeps sending till the buffer is not empty.
The method is responsible for keeping track of the size of the data to send.
*/
{
    if (this->socketFd < 0) {
        // Some error 
        LOG_ERROR("invalid sockFd in sendFile");
        return false;  
    }

    if (fileData.empty()) {
        LOG_WARNING("empty file");
        return true;
    }

    size_t fileSize = fileData.size();
    size_t remainingSize = fileSize;
    size_t offset = 0;
    uint32_t seqNo = 0;
    
    while (remainingSize > MAX_PAYLOAD_SIZE) {
        size_t thisSize = min<size_t>(MAX_PAYLOAD_SIZE, remainingSize);
        remainingSize -= thisSize;
        vector<char> packetData(thisSize);
        std::copy(fileData.begin() + offset, fileData.begin() + offset + thisSize, packetData.begin());
        offset += thisSize;
        struct packet * pkt = makePacket(packetData); // malloc here
        pkt->seqNo = seqNo++;
        if (remainingSize <= 0) {
            pkt->isLast = 1;
        } 
        vector<char> serializedPkt = serializePacket(pkt);

        computeChecksum(serializedPkt);

        if (pkt != nullptr) {
            delete pkt;
            pkt = nullptr;
        }

    }



    
    
    
}