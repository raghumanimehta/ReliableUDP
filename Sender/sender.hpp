/**
Sender class. The class encapsulates the logic for sending the file using UDP. 
 */
#ifndef SENDER_HPP          
#define SENDER_HPP  

#include <string>
#include <vector>  

class Sender {
private:
    struct sockaddr_in dst;
    int socketFd;

public:
    Sender(const std::string& destIp, const int port);
    
    ~Sender();

    bool sendFile(const std::vector<char>& fileData);

}; 

#endif