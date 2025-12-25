# C++17 Makefile: builds two executables into bin/
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -O2 -MMD -MP

BIN_DIR := bin
SENDER_BIN := $(BIN_DIR)/sender
RECEIVER_BIN := $(BIN_DIR)/receiver

SENDER_SRCS := $(wildcard Sender/*.cpp)
RECEIVER_SRCS := $(wildcard Receiver/*.cpp)
COMMON_SRCS := logger.cpp packet.cpp

SENDER_OBJS := $(SENDER_SRCS:.cpp=.o) $(COMMON_SRCS:.cpp=.o)
RECEIVER_OBJS := $(RECEIVER_SRCS:.cpp=.o) $(COMMON_SRCS:.cpp=.o)

DEPS := $(SENDER_OBJS:.o=.d) $(RECEIVER_OBJS:.o=.d)

.PHONY: all clean
all: $(SENDER_BIN) $(RECEIVER_BIN)

$(SENDER_BIN): | $(BIN_DIR)
$(RECEIVER_BIN): | $(BIN_DIR)

$(SENDER_BIN): $(SENDER_OBJS)
	$(CXX) $(SENDER_OBJS) -o $@

$(RECEIVER_BIN): $(RECEIVER_OBJS)
	$(CXX) $(RECEIVER_OBJS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BIN_DIR):
	mkdir -p $@

clean:
	rm -f $(SENDER_BIN) $(RECEIVER_BIN)
	rm -f output.txt
	find . -name '*.o' -delete
	find . -name '*.d' -delete

-include $(DEPS)