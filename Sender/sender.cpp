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
    if (socketFd < 0) {
        // Some error 
        return false;  
    }

    
    
    
}