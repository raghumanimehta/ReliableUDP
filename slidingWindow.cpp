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

bool SlidingWindow::addToWindow(WindowSlot w) 
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

bool SlidingWindow::removeFromWindow()
{
	if (slots.empty()) return false;
	auto it = slots.find(base);
	if (it != slots.end()) {
		slots.erase(it);
		++base;
		return true;
	}

	// If `base` not found, remove the slot with the smallest sequence number as a fallback.
	auto minIt = std::min_element(slots.begin(), slots.end(),
								  [](const auto &a, const auto &b){ return a.first < b.first; });
	if (minIt != slots.end()) {
		slots.erase(minIt);
		return true;
	}
	return false;
}