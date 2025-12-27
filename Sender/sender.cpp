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
    : socketFd(-1) 
{
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd == -1) {
        LOG_ERROR("Failed to create socket: " + std::string(strerror(errno)));
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }
    
    memset(&dst, 0, sizeof(dst));
    dst.sin_family = AF_INET;                    // use IPv4
    dst.sin_port   = htons(port);              
    inet_pton(AF_INET, destIp.c_str(), &dst.sin_addr); // destination IP
    LOG_INFO(std::string("Sender created with destination: ") + destIp + ":" + std::to_string(port));
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
    
    while (remainingSize > 0)
    {
        size_t thisSize = min<size_t>(MAX_PAYLOAD_SIZE, remainingSize);
        remainingSize -= thisSize;
        vector<char> packetData(thisSize);
        std::copy(fileData.begin() + offset, fileData.begin() + offset + thisSize, packetData.begin());
        offset += thisSize;
        auto pkt = makePacket(packetData); 
        if (pkt == nullptr) {
            throw std::runtime_error("make packet returned nullptr");
        }
        pkt->seqNo = seqNo++;
        if (remainingSize <= 0) {
            pkt->flag = FLAG_FIN;
        } 
        vector<char> serializedPkt = serializePacket(*pkt);

        ssize_t sentBytes = sendto(socketFd, serializedPkt.data(), serializedPkt.size(), 0,
                           (struct sockaddr*)&dst, sizeof(dst));

        if (sentBytes == -1) {
            LOG_ERROR("Failed to send packet: " + std::string(strerror(errno)));
            return false;
        }
    }

    return true; 

}