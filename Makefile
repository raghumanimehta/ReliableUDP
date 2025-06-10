# Simple C++17 Makefile for Linux/Unix
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror -O2
TARGET = program
SOURCES = $(shell find . -name "*.cpp")
OBJECTS = $(SOURCES:.cpp=.o)


$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET)


%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@


clean:
	rm -f $(OBJECTS) $(TARGET)
	@find . -name "*.o" -delete 2>/dev/null || true


.PHONY: clean