#ifndef SLIDING_WINDOW_HPP
#define SLIDING_WINDOW_HPP

#include <cstdint>
#include <vector>
#include <memory>
#include <unordered_map>
#include "packet.hpp"
#include <chrono>

constexpr size_t WINDOW_SIZE = 10;
constexpr std::chrono::milliseconds TIMEOUT = std::chrono::milliseconds(500); // Adjust as needed

struct WindowSlot {
    std::unique_ptr<packet> packet;  // The packet data
    int retransmitCount;             // Number of retransmissions
};

class SlidingWindow {
    private:
        std::unordered_map<uint32_t, WindowSlot> slots;  // key = sequence number
        uint32_t base;                                    // Oldest unacked sequence number
        uint32_t nextSeqNo;                              // Next sequence number to use
        
        // Single timer for entire window
        std::chrono::steady_clock::time_point timerStart;
        bool isTimerRunning;
        void startTimer();

    public: 
        SlidingWindow(uint32_t base);
        ~SlidingWindow();
        
        bool add(WindowSlot w);
        bool remove(uint32_t seqNo);
        
        bool isTimedOut();
        std::vector<std::unique_ptr<packet>> getPktsForRetransmit();
        
        // Inline utility functions
        inline bool isFull() const { return slots.size() >= WINDOW_SIZE; }
        inline bool isEmpty() const { return slots.empty(); }
        inline uint32_t getBase() const { return base; }
        inline size_t size() const { return slots.size(); }
        inline void stopTimer() { isTimerRunning = false; }
};

#endif