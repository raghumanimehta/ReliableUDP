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
#include "../packet.hpp"
#include "../logger.cpp"

using namespace std;

Receiver::Receiver(const int port)
    : socketFd(-1) 
{
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd == -1) {
        LOG_ERROR("Failed to create socket: " + std::string(strerror(errno)));
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);        
    addr.sin_addr.s_addr = INADDR_ANY;  
    if (bind(socketFd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        LOG_ERROR("bind failed: " + std::string(strerror(errno)));
        throw std::runtime_error("bind failed: " + std::string(strerror(errno)));
    }
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

    while ()
}