/**
Sender class. The class encapsulates the logic for sending the file using UDP. 
 */
#ifndef RECEIVER_HPP          
#define RECEIVER_HPP  

#include <string>
#include <vector>  
#include <sys/socket.h>   
#include <netinet/in.h>    
#include <arpa/inet.h>    

class Receiver 
{
private:
    struct sockaddr_in from;
    int socketFd;

public:
    Receiver(const std::string& destIp, const int port);
    
    ~Receiver();

    bool bindToPort();
};