#include "sender.hpp"

// Required for socket functions
#include <sys/socket.h>  

// Required for address structures
#include <netinet/in.h>  

// Required for inet_addr and related functions
#include <arpa/inet.h>   // For inet_addr(), inet_ntoa(), etc.

// Required for close()
#include <unistd.h>     
#include <poll.h>
#include <iostream>    
#include <string>
#include <cstring>    
#include "../packet.hpp"
#include "../logger.cpp"

using namespace std;

Sender::Sender(const std::string& destIp, const int port)
    : socketFd(-1), state(SenderState::IDLE) 
{
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd == -1)
    {
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
        LOG_ERROR("invalid sockFd in sendFile");
        return false;  
    }

    if (!this->handshake())
    {
        LOG_ERROR("Handshake failed");
        return false;
    }

    if (this->state != SenderState::CONNECTED) 
    {
        LOG_ERROR("State must be CONNECTED");
        return false;
    }

    if (fileData.empty()) 
    {
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
        uint8_t flag = (remainingSize <= 0) ? FLAG_FIN : FLAG_DATA;
        auto pkt = makePacket(packetData, seqNo, flag); 
        if (pkt == nullptr) 
        {
            LOG_ERROR("makePacket returned nullptr");
            return false;
        } 
        if (!sendPacket(*pkt)) 
        {
            return false;
        }

        seqNo++; 
    }

    return true; 

}

bool Sender::handshake()
{
    if (this->socketFd < 0) 
    {
        LOG_ERROR("invalid sockFd in sendFile");
        return false;  
    }   

    if (this->state != SenderState::IDLE)
    {
        LOG_ERROR("State must be IDLE to initiate handshake");
        return false; 
    }

    auto pkt = makeEmptyPacket();   
    pkt->seqNo = 0;
    pkt->flag = FLAG_SYN;
    if (pkt == nullptr) 
    {
        LOG_ERROR("makePacket returned nullptr");
        return false;
    }

    if (!sendPacket(*pkt)) 
    {
        return false;
    }
    this->state = SenderState::SYN_SENT;

    if (!waitForSynAck()) 
    {
        return false;
    }

    if (!sendHandshakeAck()) 
    {
        return false;
    }

    this->state = SenderState::CONNECTED;
    return true; 
}

bool Sender::waitForSynAck()
{
    struct pollfd pfd;
    pfd.fd = this->socketFd;
    pfd.events = POLLIN;

    const uint8_t MAX_RETRIES = 10;
    uint8_t retries = 0;

    while (this->state == SenderState::SYN_SENT) 
    {  
        int ret = poll(&pfd, 1, 1000); // 1 second timeout
        if (ret < 0) 
        {
            if (errno == EINTR) continue;
            LOG_ERROR("Poll error: " + std::string(strerror(errno)));
            return false; 
        }

        if (ret > 0 && (pfd.revents & POLLIN)) {
            auto pkt = readPkt();
            if (pkt == nullptr) 
            {
                LOG_ERROR("Failed to read packet during handshake");
                retries++;
                continue;
            }
            
            if (pkt->flag == FLAG_SYN_ACK && pkt->seqNo == 1) 
            {
                LOG_INFO("Received SYN-ACK, handshake complete");
                this->state = SenderState::SYN_ACK;
                return true;
            }
            LOG_WARNING("Received unexpected packet");
        }

        if (++retries >= MAX_RETRIES) 
        {
            LOG_ERROR("Handshake failed: Maximum retries reached without receiving SYN-ACK");
            return false;
        }
    }
    return true;
}

bool Sender::sendHandshakeAck()
{
    auto ackPkt = makeEmptyPacket();
    ackPkt->seqNo = 1; 
    ackPkt->flag = FLAG_ACK;
    return sendPacket(*ackPkt);
}

bool Sender::sendPacket(const packet& pkt) 
{
    vector<char> serializedPkt = serializePacket(pkt);
    ssize_t sentBytes = sendto(socketFd, serializedPkt.data(), serializedPkt.size(), 0,
                        (struct sockaddr*)&dst, sizeof(dst));
    if (sentBytes == -1) {
        LOG_ERROR("Failed to send packet: " + std::string(strerror(errno)));
        return false;
    }
    return true;
}

unique_ptr<packet> Sender::readPkt() 
{
    char buf[MAX_PACKET_SIZE];
    socklen_t addrLen = sizeof(this->dst);
    ssize_t recvBytes = recvfrom(socketFd, buf, MAX_PACKET_SIZE, 0,
                                (struct sockaddr*)&(this->dst), &addrLen);

    if (recvBytes < 0) 
    {
        LOG_ERROR("Failed to receive first packet: " + std::string(strerror(errno)));
        return nullptr; 
    }

    vector<char> data(buf, buf + recvBytes);
    auto pkt = deserializePacket(data);
    return pkt;
}