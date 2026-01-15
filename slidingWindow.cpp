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
      slots(),
      timerStart(std::chrono::steady_clock::now()),
      isTimerRunning(false)
{
    this->slots.reserve(WINDOW_SIZE); 
}

SlidingWindow::~SlidingWindow() 
{
	slots.clear();
}

void SlidingWindow::startTimer() {
    timerStart = std::chrono::steady_clock::now();
    isTimerRunning = true;
} 

bool SlidingWindow::add(WindowSlot w) 
{
	if (this->slots.size() >= WINDOW_SIZE) {
		return false;
	}

	if (this->slots.find(w.packet->seqNo) == this->slots.end())  {
        this->slots.insert(std::make_pair(w.packet->seqNo, w));

        if (!this->isTimerRunning) {
            startTimer();
            isTimerRunning = true;
        }
        nextSeqNo++;
		LOG_INFO("Added packet with sequence number: " + std::to_string(w.packet->seqNo) + " to window");
		return true;
    }
    LOG_WARNING("pkt with seqNo: " + std::to_string(w.packet->seqNo) + " already in the sliding window");
	return false;
}

bool SlidingWindow::remove(uint32_t ackSeqNo)
{
    if (slots.empty()) return false;
    
    uint32_t actualSeqNo = ackSeqNo - 1;
    bool removedAny = false;
    
    auto it = slots.begin();
    while (it != slots.end()) {
        if (it->first <= actualSeqNo) {
            LOG_INFO("Removing packet with seqNo: " + std::to_string(it->first));
            it = slots.erase(it);
            removedAny = true;
        } else {
            ++it;
        }
    }
    
    if (!slots.empty()) {
        auto minIt = std::min_element(slots.begin(), slots.end(),
                                      [](const auto &a, const auto &b){ return a.first < b.first; });
        base = minIt->first;
        startTimer();
    } else {
        base = actualSeqNo + 1;
        isTimerRunning = false;
    }
    
    return removedAny;
}

bool SlidingWindow::isTimedOut() 
{
    if (!isTimerRunning) 
    {
        LOG_WARNING("Attempting to call isTimeout when the timer is not running");
        return false;
    }
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - timerStart);
    return elapsed >= TIMEOUT;
}

std::vector<std::unique_ptr<packet>> SlidingWindow::getPktsForRetransmit()
{
    std::vector<std::unique_ptr<packet>> ret;
    for (auto& p: slots) 
    {
        auto copy = std::make_unique<packet>(*p.second.packet);
        ret.push_back(std::move(copy));
    }
    return ret;
}

    