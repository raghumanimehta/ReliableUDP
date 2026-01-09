#ifndef SLIDING_WINDOW_HPP
#define SLIDING_WINDOW_HPP

#include <cstdint>
#include <vector>
#include <memory>
#include <unordered_map>
#include "packet.hpp"

constexpr size_t WINDOW_SIZE = 10;

struct WindowSlot {
    std::unique_ptr<packet> packet;                      // The packet data
    std::chrono::steady_clock::time_point sentTime;      // When packet was sent (for timeout)
    int retransmitCount;                                 // Number of retransmissions
};

class SlidingWindow {
    private:
        std::unordered_map<uint32_t, WindowSlot> slots;  // key = sequence number
        uint32_t base;                                    // Oldest unacked sequence number
        uint32_t nextSeqNo;                              // Next sequence number to use


    public: 
    SlidingWindow(uint32_t base);
    ~SlidingWindow();
    bool addToWindow(WindowSlot w);
    bool removeFromWindow();
    
};



#endif