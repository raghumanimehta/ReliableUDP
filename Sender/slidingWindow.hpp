#ifndef SLIDING_WINDOW_HPP
#define SLIDING_WINDOW_HPP

#include "../packet.hpp"
#include <chrono>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

constexpr std::chrono::milliseconds WINDOW_TIMEOUT =
    std::chrono::milliseconds(500); // Adjust as needed

struct WindowSlot {
    std::unique_ptr<packet> pkt; // The packet data
    int retransmitCount;         // Number of retransmissions
};

class SlidingWindow {
  private:
    std::unordered_map<uint32_t, WindowSlot> slots; // key = sequence number
    uint32_t base;      // Oldest unacked sequence number
    uint32_t nextSeqNo; // Next sequence number to use
    size_t window_size;
    uint8_t maxRetransmitCount;

    // Single timer for entire window
    std::chrono::steady_clock::time_point timerStart;
    bool isTimerRunning;

  public:
    SlidingWindow(uint32_t base, size_t window_size,
                  uint8_t maxRetransmitCount);
    ~SlidingWindow();

    void startTimer();
    bool add(WindowSlot w);
    bool remove(uint32_t seqNo);

    bool isTimedOut();
    bool resetTimer();
    std::optional<std::vector<std::unique_ptr<packet>>> getPktsForRetransmit();

    // Inline utility functions
    inline bool isFull() const { return slots.size() >= window_size; }
    inline bool isEmpty() const { return slots.empty(); }
    inline uint32_t getBase() const { return base; }
    inline size_t size() const { return slots.size(); }
    inline void stopTimer() { isTimerRunning = false; }
};

#endif
