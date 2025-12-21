/**
Receiver class. The class encapsulates the logic for receiving files using UDP. 
 */
#ifndef RECEIVER_HPP          
#define RECEIVER_HPP

#include <string>
#include <vector>  
#include <netinet/in.h>

#define PORT 8080 

class Receiver {

private:
    struct sockaddr_in origin;
    int socketFd;
    bool isConnected;

public:
    Receiver(); 
    
    ~Receiver();

    bool receiveFile();  // Receive and save file

}; 

#endif