#include "receiver.hpp"

// Required for socket functions
#include <cstddef>
#include <cstdint>
#include <memory>
#include <sys/socket.h>

// Required for address structures
#include <netinet/in.h>

// Required for inet_addr and related functions
#include <arpa/inet.h> // For inet_addr(), inet_ntoa(), etc.

// Required for close()
#include <unistd.h>

#include "../logger.cpp"
#include "../packet.hpp"
#include "../utils.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <poll.h>
#include <stdio.h>
#include <string>

using namespace std;

static std::string receiverStateToString(ReceieverState s) {
    switch (s) {
    case ReceieverState::IDLE:
        return "IDLE";
    case ReceieverState::SYN_RECV:
        return "SYN_RECV";
    case ReceieverState::ACK_SENT:
        return "ACK_SENT";
    case ReceieverState::CONNECTED:
        return "CONNECTED";
    case ReceieverState::CLOSING_INIT:
        return "CLOSING_INIT";
    case ReceieverState::CLOSING:
        return "CLOSING";
    case ReceieverState::CLOSED:
        return "CLOSED";
    default:
        return "UNKNOWN";
    }
}

Receiver::Receiver() : socketFd(-1), expectedSeqNo(0) {
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd == -1) {
        LOG_ERROR("[RECEIVER-INIT] Failed to create UDP socket. Error: " +
                  std::string(strerror(errno)));
        throw std::runtime_error("Failed to create socket: " +
                                 std::string(strerror(errno)));
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (::bind(socketFd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        LOG_ERROR("[RECEIVER-INIT] Bind failed on port " +
                  std::to_string(PORT) +
                  ". Error: " + std::string(strerror(errno)));
        throw std::runtime_error("bind failed: " +
                                 std::string(strerror(errno)));
    }
    origin = {};
    state = ReceieverState::IDLE;
    LOG_INFO(
        "[RECEIVER-INIT] Successfully initialized. State: IDLE | Socket FD: " +
        std::to_string(socketFd) + " | Bound to port: " + std::to_string(PORT));
}

Receiver::~Receiver() {
    if (socketFd >= 0) {
        close(socketFd);
    }
}

unique_ptr<packet> Receiver::receivePkt() {
    // TODO: perhaps make the following constants
    //
    const uint64_t TIMEOUT_MS = 1000;
    const uint32_t RETRIES = 1000;

    if (waitForReadWithRetry(socketFd, TIMEOUT_MS, RETRIES) != SUCCESS) {
        LOG_ERROR(
            "[RECEIVE-FILE] Timeout or error waiting for data packet. State: " +
            receiverStateToString(this->state));
        return nullptr;
    }

    auto pkt = readPkt(socketFd, origin);
    if (pkt == nullptr) {
        LOG_ERROR("[RECEIVE-FILE] Failed to read data packet. State: " +
                  receiverStateToString(this->state));
        return nullptr;
    }
    return pkt;
}

bool Receiver::sendAck(uint32_t seqNo) {
    auto ackPkt = makeEmptyPacket();
    ackPkt->flag = FLAG_ACK;
    ackPkt->seqNo = seqNo + 1;
    LOG_INFO("[SEND-ACK] Sending ACK packet. State: " +
             receiverStateToString(this->state) +
             " | AckSeqNo: " + std::to_string(ackPkt->seqNo));
    return sendPacket(this->socketFd, this->origin, *ackPkt);
}

bool Receiver::isBufferFull() const {
    return this->rxBuffer.size() >= RX_WINDOW_SIZE;
}

bool Receiver::addToBuffer(std::unique_ptr<packet> pkt) {
    if (pkt == nullptr) {
        LOG_ERROR("[ADD-TO-BUFFER] Cannot add null packet to buffer.");
        return false;
    }
    if (this->isBufferFull()) {
        LOG_WARNING(
            "[ADD-TO-BUFFER] Buffer is full. Cannot add packet with SeqNo: " +
            std::to_string(pkt->seqNo));
        return false;
    }
    uint32_t seqNo = pkt->seqNo;
    if (this->rxBuffer.find(seqNo) != this->rxBuffer.end()) {
        LOG_WARNING("[ADD-TO-BUFFER] Packet with SeqNo: " +
                    std::to_string(seqNo) + " already in buffer.");
        return false;
    }
    this->rxBuffer[seqNo] = std::move(pkt);
    LOG_INFO("[ADD-TO-BUFFER] Added packet to buffer. SeqNo: " + std::to_string(seqNo));
    return true;
}

void Receiver::drainBufferedPackets(std::ofstream& outFile) {
    while (this->rxBuffer.find(this->expectedSeqNo) != this->rxBuffer.end()) {
        auto& bufferedPkt = this->rxBuffer[this->expectedSeqNo];
        
        outFile.write(bufferedPkt->payload, bufferedPkt->payloadLen);
        LOG_INFO("[RECEIVE-FILE] Wrote packet payload. SeqNo: " +
                 std::to_string(this->expectedSeqNo) + " | Bytes: " + std::to_string(bufferedPkt->payloadLen));

        bool isFin = (bufferedPkt->flag & FLAG_FIN) != 0;
        this->rxBuffer.erase(this->expectedSeqNo);
        this->expectedSeqNo++;

        if (isFin) {
            this->state = ReceieverState::CLOSED;
            break;
        }
    }
}

bool Receiver::receiveFile() {
    LOG_INFO("[RECEIVE-FILE] Starting file reception.\n"
             "  State: " +
             receiverStateToString(this->state));
    if (!handshake()) {
        LOG_ERROR("[RECEIVE-FILE] Handshake failed. State: " +
                  receiverStateToString(this->state));
        return false;
    }

    LOG_INFO("[RECEIVE-FILE] Handshake completed.\n"
             "  State: " +
             receiverStateToString(this->state) +
             "\n  Waiting for data packets");

    std::ofstream outFile("output.txt", std::ios::binary);
    if (!outFile.is_open()) {
        LOG_ERROR("[RECEIVE-FILE] Failed to open output.txt for writing");
        return false;
    }

    while (this->state == ReceieverState::CONNECTED) {
        auto pkt = receivePkt();
        if (pkt == nullptr) {
            LOG_ERROR("[RECEIVE-FILE] Failed to receive packet (timeout or "
                      "error). State: " +
                      receiverStateToString(this->state));
            outFile.close();
            return false;
        }

        uint32_t seqNo = pkt->seqNo;

        // Case A: Duplicate Packet (too old)
        if (seqNo < this->expectedSeqNo) {
            LOG_WARNING("[RECEIVE-FILE] Duplicate packet received. SeqNo: " +
                        std::to_string(seqNo) +
                        " | Expected: " + std::to_string(this->expectedSeqNo));
            this->sendAck(seqNo);
            continue;
        }

        // Case B: Out-of-Window Packet (too new/invalid)
        if (seqNo >= this->expectedSeqNo + RX_WINDOW_SIZE) {
            LOG_WARNING(
                "[RECEIVE-FILE] Out-of-window packet received. SeqNo: " +
                std::to_string(seqNo) +
                " | Expected: " + std::to_string(this->expectedSeqNo));
            continue;
        }

        // Case C: Valid packet (within window). Add to buffer.
        this->addToBuffer(std::move(pkt));

        // Process contiguous packets starting from expectedSeqNo
        this->drainBufferedPackets(outFile);

        // Send cumulative ACK (which is expectedSeqNo - 1)
        this->sendAck(this->expectedSeqNo - 1);
    }

    outFile.close();
    LOG_INFO("[RECEIVE-FILE] File reception completed successfully.\n"
             "  State: " +
             receiverStateToString(this->state));
    return true;
}

bool Receiver::waitAndUpdateState(uint64_t timeout, uint32_t retries,
                                  uint32_t expectedSeqNo, uint8_t expectedFlag,
                                  ReceieverState nextState) {
    if (waitForReadWithRetry(socketFd, timeout, retries) != SUCCESS) {
        LOG_ERROR("[HANDSHAKE] Timeout waiting for SYN packet. State: " +
                  receiverStateToString(this->state));
        return false;
    }

    auto pkt = readPkt(socketFd, origin);
    if (pkt == nullptr) {
        LOG_ERROR("[HANDSHAKE] Failed to read SYN packet. State: " +
                  receiverStateToString(this->state));
        return false;
    }

    if (pkt->flag == expectedFlag && pkt->seqNo == expectedSeqNo) {
        this->state = nextState;
        return true;
    } else {
        LOG_ERROR(
            "[HANDSHAKE] Received packet with unexpected values. State: " +
            receiverStateToString(this->state) +
            " | SeqNo: " + std::to_string(pkt->seqNo) +
            " (Expected: " + std::to_string(SYN_SEQNO) +
            ") | Flag: " + std::to_string(pkt->flag) + " (Expected: SYN)");
        return false;
    }
}

bool Receiver::handshake() {
    LOG_INFO("[HANDSHAKE] Starting handshake.\n"
             "  State: " +
             receiverStateToString(this->state) +
             "\n  Waiting for SYN packet from sender");

    const uint64_t TIMEOUT_MS = 1000;
    const uint8_t RETRIES = 10;
    if (this->waitAndUpdateState(TIMEOUT_MS, RETRIES, this->expectedSeqNo,
                                 FLAG_SYN, ReceieverState::SYN_RECV)) {
        LOG_INFO("[HANDSHAKE] Received valid SYN packet.\n"
                 "  State: " +
                 receiverStateToString(this->state) +
                 " -> SYN_RECV"
                 "\n  SeqNo: " +
                 std::to_string(this->expectedSeqNo) + "\n  Flag: SYN");
        this->expectedSeqNo++;
    } else {
        return false;
    }
    auto sendPkt = makeEmptyPacket();
    sendPkt->flag = FLAG_SYN_ACK;
    sendPkt->seqNo = this->expectedSeqNo;
    LOG_INFO("[HANDSHAKE] Sending SYN-ACK response.\n"
             "  State: " +
             receiverStateToString(this->state) +
             " -> ACK_SENT"
             "\n  SeqNo: " +
             std::to_string(sendPkt->seqNo) + "\n  Flag: SYN_ACK");
    if (!sendPacket(this->socketFd, this->origin, *sendPkt)) {
        LOG_ERROR("[HANDSHAKE] Failed to send SYN-ACK packet. State: " +
                  receiverStateToString(this->state));
        return false;
    }
    this->state = ReceieverState::ACK_SENT;

    if (this->waitAndUpdateState(TIMEOUT_MS, RETRIES, this->expectedSeqNo,
                                 FLAG_ACK, ReceieverState::CONNECTED)) {
        LOG_INFO("[HANDSHAKE] Received valid ACK packet.\n"
                 "  State: " +
                 receiverStateToString(this->state) +
                 " -> CONNECTED"
                 "\n  SeqNo: " +
                 std::to_string(this->expectedSeqNo) + "\n  Flag: ACK");
        this->expectedSeqNo++;
    } else {
        return false;
    }
    LOG_INFO("[HANDSHAKE] Three-way handshake completed successfully.\n"
             "  State: " +
             receiverStateToString(this->state));
    return true;
}
