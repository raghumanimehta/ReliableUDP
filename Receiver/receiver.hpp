/**
Receiver class. The class encapsulates the logic for receiving files using UDP.
 */
#ifndef RECEIVER_HPP
#define RECEIVER_HPP

#include "../packet.hpp"
#include <cstdint>
#include <fstream>
#include <map>
#include <netinet/in.h>
#include <stdio.h>

#define PORT 8080

constexpr size_t RX_WINDOW_SIZE = 10;

enum class ReceieverState : uint8_t {
    IDLE = 0,
    SYN_RECV = 1,
    ACK_SENT = 2,
    CONNECTED = 3,
    CLOSING_INIT = 4,
    CLOSING = 5,
    CLOSED = 6
};

class Receiver {
  private:
    struct sockaddr_in origin;
    int socketFd;
    ReceieverState state;
    std::map<uint32_t, std::unique_ptr<packet>> rxBuffer;
    uint32_t expectedSeqNo;
    bool handshake();
    bool waitAndUpdateState(uint64_t timeout, uint32_t retries,
                            uint32_t expectedSeqNo, uint8_t expectedFlag,
                            ReceieverState nextState);
    std::unique_ptr<packet> receivePkt();
    bool sendAck(uint32_t seqNo);
    bool isBufferFull() const;
    bool addToBuffer(std::unique_ptr<packet> pkt);
    void drainBufferedPackets(std::ofstream& outFile);

  public:
    Receiver();

    ~Receiver();

    bool receiveFile(); // Receive and save file
};

#endif
