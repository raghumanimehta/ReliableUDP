#include "slidingWindow.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>
#include <memory>
#include <unordered_map>
#include <sstream>
#include "packet.hpp"
#include "logger.cpp"

SlidingWindow::SlidingWindow(uint32_t base)
    : base(base), 
      nextSeqNo(0), 
      slots() 
{
    this->slots.reserve(WINDOW_SIZE); 
}

SlidingWindow::~SlidingWindow() 
{
	slots.clear();
}

bool SlidingWindow::add(WindowSlot w) 
{
	if (this->slots.size() >= WINDOW_SIZE) {
		return false;
	}

	if (this->slots.find(w.packet->seqNo) == this->slots.end())  {
        this->slots.insert(std::make_pair(w.packet->seqNo, w));
		LOG_INFO("Added packet with sequence number: " + std::to_string(w.packet->seqNo) + " to window");
		return true;
    }
    LOG_WARNING("pkt with seqNo: " + std::to_string(w.packet->seqNo) + " already in the sliding window");
	return false;
}

bool SlidingWindow::remove(uint32_t ackSeqNo)
{
    if (slots.empty()) return false;
    
    // TCP uses cumulative ACKs: ackSeqNo acknowledges all packets < ackSeqNo
    uint32_t actualSeqNo = ackSeqNo - 1;
    
    bool removedAny = false;
    
    // Remove all packets with seqNo <= actualSeqNo
    auto it = slots.begin();
    while (it != slots.end()) {
        if (it->first <= actualSeqNo) {
            LOG_INFO("Removing packet with seqNo: " + std::to_string(it->first));
            
            // Update base to the next packet after removed ones
            if (it->first == base) {
                // Find next base after removal
                uint32_t oldBase = base;
                base = actualSeqNo + 1;  // Move base forward
                
                // Look for the actual next unacked packet
                for (uint32_t seq = oldBase + 1; seq <= actualSeqNo + WINDOW_SIZE; ++seq) {
                    if (slots.find(seq) != slots.end()) {
                        base = seq;
                        break;
                    }
                }
            }
            
            it = slots.erase(it);  // erase returns iterator to next element
            removedAny = true;
        } else {
            ++it;
        }
    }
    
    // Update base to smallest remaining seqNo if window not empty
    if (!slots.empty()) {
        auto minIt = std::min_element(slots.begin(), slots.end(),
                                      [](const auto &a, const auto &b){ return a.first < b.first; });
        base = minIt->first;
    }
    
    return removedAny;
}