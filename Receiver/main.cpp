/* 
Main file for the Receiver application.
This file contains the main function that initializes the application and starts the event loop.
*/
#include "../logger.cpp"
#include "receiver.hpp"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <string>

static std::string getReceiverIpAddress() {
    struct ifaddrs *ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1) {
        return "127.0.0.1";
    }

    std::string ipAddress = "127.0.0.1";
    for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }

        if ((ifa->ifa_flags & IFF_LOOPBACK) != 0) {
            continue;
        }

        char addrBuffer[INET_ADDRSTRLEN];
        auto *addr = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
        if (inet_ntop(AF_INET, &(addr->sin_addr), addrBuffer,
                      sizeof(addrBuffer)) != nullptr) {
            ipAddress = addrBuffer;
            break;
        }
    }

    freeifaddrs(ifaddr);
    return ipAddress;
}

int main() {
    // argc: number of arguments
    // argv: array of C-style strings (arguments)
    // if (argc != 2) 
    // {
    //     LOG_ERROR("There must be exactly one argument: path to save the received file.");
    //     std::cout << "Usage: " << argv[0] << " <output_file_path>" << std::endl;
    //     return 1; 
    // } 
    // Receiver receiver(argv[1]);
    // receiver.receiveFile();
    Receiver receiver;
    LOG_INFO("[RECEIVER-MAIN] Sender destination IP: " + getReceiverIpAddress() +
             " | Port: " + std::to_string(PORT));
    bool recieved = receiver.receiveFile();
    if (recieved == false) 
    {
        LOG_ERROR("Unable to receive file"); 
        return 1; 
    }
    return 0;
}
