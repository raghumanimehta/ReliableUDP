#include "sender.hpp"

// Required for socket functions
#include <memory>
#include <sys/socket.h>

// Required for address structures
#include <netinet/in.h>

// Required for inet_addr and related functions
#include <arpa/inet.h> // For inet_addr(), inet_ntoa(), etc.

// Required for close()
#include "../logger.cpp"
#include "../packet.hpp"
#include "../utils.hpp"
#include <cstring>
#include <poll.h>
#include <string>
#include <unistd.h>

using namespace std;

const uint64_t TIMEOUT_MS = 1000;

static std::string senderStateToString(SenderState s) {
    switch (s) {
    case SenderState::IDLE:
        return "IDLE";
    case SenderState::SYN_SENT:
        return "SYN_SENT";
    case SenderState::SYN_ACK:
        return "SYN_ACK";
    case SenderState::CONNECTED:
        return "CONNECTED";
    case SenderState::CLOSING:
        return "CLOSING";
    case SenderState::CLOSED:
        return "CLOSED";
    default:
        return "UNKNOWN";
    }
}

Sender::Sender(const std::string &destIp, const int port)
    : socketFd(-1), state(SenderState::IDLE),
      window(0, WINDOW_SIZE, MAX_RETRANSMIT_COUNT) {
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd == -1) {
        LOG_ERROR("[SENDER-INIT] Failed to create UDP socket. Error: " +
                  std::string(strerror(errno)));
        throw std::runtime_error("Failed to create socket: " +
                                 std::string(strerror(errno)));
    }

    memset(&dst, 0, sizeof(dst));
    dst.sin_family = AF_INET; // use IPv4
    dst.sin_port = htons(port);
    inet_pton(AF_INET, destIp.c_str(), &dst.sin_addr); // destination IP
    LOG_INFO(
        std::string("[SENDER-INIT] Successfully initialized. State: IDLE | "
                    "Destination: ") +
        destIp + ":" + std::to_string(port) +
        " | Socket FD: " + std::to_string(socketFd));
}

Sender::~Sender() {
    if (socketFd >= 0) {
        close(socketFd);
    }
}

bool Sender::sendFile(const std::vector<char> &fileData)
/*
Sends the file over UDP connection.
Currently basic version only. Keeps sending till the buffer is not empty.
The method is responsible for keeping track of the size of the data to send.
*/
{
    if (this->socketFd < 0) {
        LOG_ERROR(
            "[SEND-FILE] Invalid socket file descriptor. State: " +
            std::string(this->state == SenderState::IDLE ? "IDLE" : "OTHER"));
        return false;
    }

    if (!this->handshake()) {
        LOG_ERROR("[SEND-FILE] Handshake failed. State: " +
                  senderStateToString(this->state));
        return false;
    }

    if (this->state != SenderState::CONNECTED) {
        LOG_ERROR(
            "[SEND-FILE] State machine error. Expected: CONNECTED, Actual: " +
            senderStateToString(this->state));
        return false;
    }

    if (fileData.empty()) {
        LOG_WARNING(
            "[SEND-FILE] Empty file data provided. State: CONNECTED | File "
            "size: 0 bytes");
        return true;
    }

    size_t fileSize = fileData.size();
    size_t remainingSize = fileSize;
    size_t offset = 0;
    uint32_t seqNo = SYN_SEQNO + 2;
    LOG_INFO("[SEND-FILE] Starting file transmission. State: CONNECTED | Total "
             "size: " +
             std::to_string(fileSize) + " bytes");

    while (remainingSize > 0) {
        size_t thisSize = min<size_t>(MAX_PAYLOAD_SIZE, remainingSize);
        remainingSize -= thisSize;
        vector<char> packetData(thisSize);
        std::copy(fileData.begin() + offset,
                  fileData.begin() + offset + thisSize, packetData.begin());
        offset += thisSize;
        uint8_t flag = (remainingSize <= 0) ? FLAG_FIN : FLAG_DATA;
        auto pkt = makePacket(packetData, seqNo, flag);
        if (pkt == nullptr) {
            LOG_ERROR("[SEND-FILE] Packet creation failed. SeqNo: " +
                      std::to_string(seqNo) +
                      " | Payload size: " + std::to_string(thisSize) +
                      " | Flag: " + std::to_string(flag));
            return false;
        }
        if (this->sendTrackedPacket(pkt)) {
            LOG_INFO(
                "[SEND-FILE] Packet sent successfully. SeqNo: " +
                std::to_string(seqNo) +
                " | Payload size: " + std::to_string(thisSize) +
                " | Flag: " + std::string(flag == FLAG_FIN ? "FIN" : "DATA") +
                " | Remaining: " + std::to_string(remainingSize) + " bytes");
        } else {
            LOG_ERROR("[SEND-FILE] Failed to send tracked packet. SeqNo: " +
                      std::to_string(seqNo) +
                      " | Payload size: " + std::to_string(thisSize));
            return false;
        }
        seqNo++;
    }

    if (!this->drainOutstandingPackets()) {
        LOG_WARNING("[SEND-FILE] Timed out before all outstanding packets were "
                    "ACKed. State: CONNECTED | Remaining window size: " +
                    std::to_string(this->window.size()));
        return false;
    }

    LOG_INFO("[SEND-FILE] File transmission completed successfully. State: "
             "CONNECTED | Total packets sent: " +
             std::to_string(seqNo));
    return true;
}

bool Sender::sendTrackedPacket(std::unique_ptr<packet> &pkt) {
    if (pkt == nullptr) {
        LOG_ERROR("[SEND-TRACKED-PACKET] Cannot send null packet.");
        return false;
    }

    while (this->window.isFull()) {
        LOG_INFO("[SEND-TRACKED-PACKET] Window full. SeqNo: " +
                 std::to_string(pkt->seqNo) + " | Waiting for ACK.");
        if (!this->waitForWindowProgress()) {
            LOG_ERROR(
                "[SEND-TRACKED-PACKET] Failed while waiting for window to "
                "make progress. SeqNo: " +
                std::to_string(pkt->seqNo));
            return false;
        }
    }

    const uint32_t seqNo = pkt->seqNo;
    const uint16_t payloadLen = pkt->payloadLen;
    if (!sendPacket(this->socketFd, this->dst, pkt)) {
        LOG_ERROR("[SEND-TRACKED-PACKET] Failed to send packet. SeqNo: " +
                  std::to_string(seqNo) +
                  " | Payload size: " + std::to_string(payloadLen));
        return false;
    }

    WindowSlot ws{std::move(pkt), 0};
    if (!this->window.add(std::move(ws))) {
        LOG_ERROR(
            "[SEND-TRACKED-PACKET] Failed to add packet to window. SeqNo: " +
            std::to_string(seqNo));
        return false;
    }

    return true;
}

bool Sender::sendUnackedPkts() {
    std::optional<std::vector<std::unique_ptr<packet>>> pkts =
        this->window.getPktsForRetransmit();
    if (!pkts.has_value()) {
        return false;
    }
    LOG_INFO("[SEND-UNACKED-PKTS] Retransmitting " +
             std::to_string(pkts->size()) + " packets from window.");
    for (auto &pkt : *pkts) {
        if (!sendPacket(this->socketFd, this->dst, pkt)) {
            LOG_ERROR("[SEND-UNACKED-PKTS] Failed to send packet during "
                      "retransmission. SeqNo: " +
                      std::to_string(pkt->seqNo));
            return false;
        }
    }
    return true;
}

bool Sender::waitForWindowProgress() {
    uint32_t retries = 1000;

    LOG_INFO("[WAIT-WINDOW] Waiting for data ACK. State: " +
             senderStateToString(this->state) +
             " | Timeout: 100ms poll | Max Retries: 1000 countdown");

    while (retries > 0) {
        if (this->window.isTimedOut()) {
            if (!this->sendUnackedPkts()) {
                LOG_ERROR("[WAIT-WINDOW] Retransmission failed or max retransmit limit reached. Aborting.");
                return false;
            }
            this->window.resetTimer();
            retries = 1000;
        }
        POLL_STATE pollState = waitForRead(socketFd, 100);
        if (pollState == INTR) {
            continue;
        }

        if (pollState == FAIL) {
            LOG_ERROR("[WAIT-WINDOW] Poll failed while waiting for data ACK. "
                      "State: " +
                      senderStateToString(this->state));
            return false;
        }

        if (pollState == TIMEOUT) {
            retries--;
            continue;
        }

        auto pkt = readPkt(this->socketFd, this->dst);
        if (pkt == nullptr) {
            LOG_ERROR("[WAIT-WINDOW] Failed to read packet while waiting for "
                      "data ACK. State: " +
                      senderStateToString(this->state));
            return false;
        }

        if (pkt->flag != FLAG_ACK) {
            LOG_WARNING("[WAIT-WINDOW] Ignoring non-ACK packet during data "
                        "transfer. SeqNo: " +
                        std::to_string(pkt->seqNo) +
                        " | Flag: " + std::to_string(pkt->flag));
            continue;
        }

        if (this->window.remove(pkt->seqNo)) {
            LOG_INFO("[WAIT-WINDOW] Received cumulative ACK. AckSeqNo: " +
                     std::to_string(pkt->seqNo) +
                     " | Removed packet(s) from sliding window. Current "
                     "window size: " +
                     std::to_string(this->window.size()));
            return true;
        }

        LOG_WARNING("[WAIT-WINDOW] Received ACK with no window progress. "
                    "AckSeqNo: " +
                    std::to_string(pkt->seqNo) + " | Current window size: " +
                    std::to_string(this->window.size()));
    }

    LOG_ERROR("[WAIT-WINDOW] Retry attempts exhausted while waiting for data "
              "ACK. State: " +
              senderStateToString(this->state));
    return false;
}

bool Sender::drainOutstandingPackets() {
    LOG_INFO("[DRAIN-WINDOW] Draining outstanding packets. State: " +
             senderStateToString(this->state) +
             " | Initial window size: " + std::to_string(this->window.size()));

    while (!this->window.isEmpty()) {
        if (!this->waitForWindowProgress()) {
            LOG_WARNING("[DRAIN-WINDOW] Timed out while waiting for remaining "
                        "packet ACKs. Remaining window size: " +
                        std::to_string(this->window.size()));
            return false;
        }
    }

    LOG_INFO("[DRAIN-WINDOW] All outstanding packets ACKed. State: " +
             senderStateToString(this->state));
    return true;
}

bool Sender::handshake() {
    if (this->socketFd < 0) {
        LOG_ERROR("[HANDSHAKE] Invalid socket file descriptor. State: IDLE");
        return false;
    }

    if (this->state != SenderState::IDLE) {
        LOG_ERROR("[HANDSHAKE] State machine error. Expected: IDLE, Actual: " +
                  senderStateToString(this->state));
        return false;
    }

    auto pkt = makeEmptyPacket();
    pkt->seqNo = SYN_SEQNO;
    pkt->flag = FLAG_SYN;
    if (pkt == nullptr) {
        LOG_ERROR("[HANDSHAKE] Packet creation failed for SYN");
        return false;
    }

    LOG_INFO(
        "[HANDSHAKE] Sending SYN packet. State: IDLE -> SYN_SENT | SeqNo: " +
        std::to_string(pkt->seqNo));
    if (!sendPacket(this->socketFd, this->dst, pkt)) {
        LOG_ERROR("[HANDSHAKE] Failed to send SYN packet. State: IDLE");
        return false;
    }

    this->state = SenderState::SYN_SENT;
    LOG_INFO("[HANDSHAKE] State transition: SYN_SENT | Waiting for SYN-ACK "
             "response");

    if (!waitForSynAck()) {
        LOG_ERROR("[HANDSHAKE] Failed to receive SYN-ACK. State: SYN_SENT");
        return false;
    }

    if (!sendHandshakeAck()) {
        LOG_ERROR("[HANDSHAKE] Failed to send final ACK. State: SYN_ACK");
        return false;
    }

    this->state = SenderState::CONNECTED;
    LOG_INFO("[HANDSHAKE] Handshake completed successfully. State: CONNECTED");
    return true;
}

bool Sender::waitForSynAck() {
    if (this->state != SenderState::SYN_SENT) {
        LOG_ERROR(
            "[WAIT-SYNACK] State machine error. Expected: SYN_SENT, Actual: " +
            senderStateToString(this->state));
        return false;
    }

    uint32_t retries = 0;
    const uint32_t maxHandshakeRetries = 5;

    LOG_INFO("[WAIT-SYNACK] Waiting for SYN-ACK response. State: SYN_SENT | "
             "Timeout: " + std::to_string(TIMEOUT_MS) + "ms | Max Handshake Retries: " +
             std::to_string(maxHandshakeRetries));

    while (retries < maxHandshakeRetries) {
        POLL_STATE pollState = waitForRead(socketFd, TIMEOUT_MS);
        if (pollState == SUCCESS) {
            auto pkt = readPkt(socketFd, dst);
            if (pkt == nullptr) {
                LOG_ERROR("[WAIT-SYNACK] Failed to read SYN-ACK packet. State: SYN_SENT");
                return false;
            }
            if (pkt->flag == FLAG_SYN_ACK && pkt->seqNo == (SYN_SEQNO + 1)) {
                LOG_INFO("[WAIT-SYNACK] Received valid SYN-ACK. State: SYN_SENT -> SYN_ACK | SeqNo: " +
                         std::to_string(pkt->seqNo) + " | Flag: SYN_ACK");
                this->state = SenderState::SYN_ACK;
                return true;
            }
            LOG_WARNING("[WAIT-SYNACK] Received unexpected packet. SeqNo: " +
                        std::to_string(pkt->seqNo) + " | Flag: " + std::to_string(pkt->flag));
        } else if (pollState == TIMEOUT) {
            retries++;
            LOG_WARNING("[WAIT-SYNACK] Timeout waiting for SYN-ACK. Retransmitting SYN packet. Retry: " +
                        std::to_string(retries) + "/" + std::to_string(maxHandshakeRetries));
            auto synPkt = makeEmptyPacket();
            synPkt->seqNo = SYN_SEQNO;
            synPkt->flag = FLAG_SYN;
            if (!sendPacket(this->socketFd, this->dst, synPkt)) {
                LOG_ERROR("[WAIT-SYNACK] Failed to retransmit SYN packet.");
                return false;
            }
        } else if (pollState == FAIL) {
            LOG_ERROR("[WAIT-SYNACK] Poll failed waiting for SYN-ACK.");
            return false;
        }
    }

    LOG_ERROR("[WAIT-SYNACK] Max handshake retry attempts exhausted. Handshake failed.");
    return false;
}

bool Sender::sendHandshakeAck() {
    auto ackPkt = makeEmptyPacket();
    ackPkt->seqNo = 1;
    ackPkt->flag = FLAG_ACK;
    LOG_INFO("[SEND-ACK] Sending final ACK packet. State: SYN_ACK | SeqNo: " +
             std::to_string(ackPkt->seqNo) + " | Flag: ACK");
    if (!sendPacket(this->socketFd, this->dst, ackPkt)) {
        LOG_ERROR("[SEND-ACK] Failed to send ACK packet. State: SYN_ACK");
        return false;
    }
    LOG_INFO("[SEND-ACK] ACK packet sent successfully");
    return true;
}
