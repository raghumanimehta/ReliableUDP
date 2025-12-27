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
    char buf[MAX_PACKET_SIZE];
    struct pollfd pfd;
    pfd.fd = socketFd;
    pfd.events = POLLIN;

    LOG_INFO("Waiting for connection (first packet)...");

    while (true)
    {  
        int ret = poll(&pfd, 1, 1000); // 1 second timeout
        if (ret < 0) {
            if (errno == EINTR) continue; // Signal interruption, retry
            LOG_ERROR("Poll error: " + std::string(strerror(errno)));
            return false; 
        }

        if (ret > 0 && (pfd.revents & POLLIN)) {
            break; 
        }    
    }

    
    socklen_t addrLen = sizeof(origin);
    ssize_t recvBytes = recvfrom(socketFd, buf, MAX_PACKET_SIZE, 0,
                                (struct sockaddr*)&origin, &addrLen);

    if (recvBytes < 0) {
        LOG_ERROR("Failed to receive first packet: " + std::string(strerror(errno)));
        return false;
    }

    vector<char> data(buf, buf + recvBytes);
    auto pkt = deserializePacket(data);
    char recvPayload[MAX_PAYLOAD_SIZE];
    memcpy(recvPayload, pkt->payload, pkt->payloadLen);
    // TODO: Handle the checks on the packet here


    LOG_INFO("Connection established. Received " + std::to_string(recvBytes) + " bytes.");

    // Write the raw received data to output.txt to verify UDP transmission
    std::ofstream outFile("output.txt", std::ios::binary);
    if (outFile.is_open()) {
        outFile.write(recvPayload, pkt->payloadLen);
        outFile.close();
        LOG_INFO("Successfully wrote received data to output.txt");
    } else {
        LOG_ERROR("Failed to open output.txt for writing");
    }

    return true;
}