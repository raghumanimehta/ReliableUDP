/**
Receiver class. The class encapsulates the logic for receiving files using UDP. 
 */
#ifndef RECEIVER_HPP          
#define RECEIVER_HPP

#include <string>
#include <vector>  
#include <netinet/in.h>

#define PORT 8080 

enum class ReceieverState : uint8_t {
    IDLE = 0,
    SYN_RECV = 1,
    ACK_SENT = 2,
    CONNECTED = 3,
    CLOSING_INIT = 4,
    CLOSING = 5,
    CLOSED = 6  
};

class Receiver 
{
private:
    struct sockaddr_in origin;
    int socketFd;
    ReceieverState state;
    bool handshake(); 

public:
    Receiver(); 
    
    ~Receiver();

    bool receiveFile();  // Receive and save file

}; 

#endif