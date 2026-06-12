#include "slidingWindow.hpp"
#include "../logger.cpp"
#include "../packet.hpp"
#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

// local timeout used by the sliding window timer
static const std::chrono::milliseconds TIMEOUT_MS =
    std::chrono::milliseconds(1000);

SlidingWindow::SlidingWindow(uint32_t base, size_t window_size,
                             uint8_t maxRetransmitCount)
    : slots(), base(base), nextSeqNo(0), maxRetransmitCount(maxRetransmitCount),
      timerStart(std::chrono::steady_clock::now()), isTimerRunning(false) {
    this->window_size = window_size;
    this->slots.reserve(window_size);
}

SlidingWindow::~SlidingWindow() { slots.clear(); }

void SlidingWindow::startTimer() {
    timerStart = std::chrono::steady_clock::now();
    isTimerRunning = true;
}

bool SlidingWindow::add(WindowSlot w) {
    if (w.pkt == nullptr) {
        LOG_ERROR("Attempted to add a null packet to the sliding window");
        return false;
    }

    if (this->slots.size() >= this->window_size) {
        return false;
    }

    const uint32_t seqNo = w.pkt->seqNo;

    if (this->slots.find(seqNo) == this->slots.end()) {
        this->slots.emplace(seqNo, std::move(w));

        if (!this->isTimerRunning) {
            startTimer();
            isTimerRunning = true;
        }
        nextSeqNo++;
        LOG_INFO("Added packet with sequence number: " + std::to_string(seqNo) +
                 " to window");
        return true;
    }
    LOG_WARNING("pkt with seqNo: " + std::to_string(seqNo) +
                " already in the sliding window");
    return false;
}

bool SlidingWindow::remove(uint32_t ackSeqNo) {
    if (slots.empty())
        return false;

    uint32_t actualSeqNo = ackSeqNo - 1;
    bool removedAny = false;

    auto it = slots.begin();
    while (it != slots.end()) {
        if (it->first <= actualSeqNo) {
            LOG_INFO("Removing packet with seqNo: " +
                     std::to_string(it->first));
            it = slots.erase(it);
            removedAny = true;
        } else {
            ++it;
        }
    }

    if (!slots.empty()) {
        auto minIt = std::min_element(
            slots.begin(), slots.end(),
            [](const auto &a, const auto &b) { return a.first < b.first; });
        base = minIt->first;
        startTimer();
    } else {
        base = actualSeqNo + 1;
        isTimerRunning = false;
    }

    return removedAny;
}

bool SlidingWindow::isTimedOut() {
    if (!isTimerRunning) {
        LOG_WARNING(
            "Attempting to call isTimeout when the timer is not running");
        return false;
    }
    auto now = std::chrono::steady_clock::now();
    auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - timerStart);
    return elapsed >= TIMEOUT_MS;
}

std::optional<std::vector<std::unique_ptr<packet>>>
SlidingWindow::getPktsForRetransmit() {
    std::vector<std::unique_ptr<packet>> ret;
    for (auto &[seqNo, ws] : slots) {
        if (ws.retransmitCount == this->maxRetransmitCount) {
            LOG_ERROR("[SLIDING-WINDOW] Packet reached max retransmission count. SeqNo: " +
                      std::to_string(seqNo) + " | Max: " + std::to_string(this->maxRetransmitCount));
            return std::nullopt;
        }
        auto copy = std::make_unique<packet>(*ws.pkt);
        ws.retransmitCount++;
        LOG_INFO("[SLIDING-WINDOW] Preparing retransmission for packet. SeqNo: " + std::to_string(seqNo) +
                 " | Attempt: " + std::to_string(ws.retransmitCount) + "/" + std::to_string(this->maxRetransmitCount));
        ret.push_back(std::move(copy));
    }
    return ret;
}

bool SlidingWindow::resetTimer() {
    if (!isTimerRunning) {
        LOG_WARNING("resetTimer called when timer was off.");
        return false;
    }
    this->startTimer();
    return true;
}
