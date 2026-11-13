CXX      = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pthread -O2 -Iinclude
TARGET   = simulator
SRC      = src/main.cpp src/pcb.cpp src/scheduler.cpp src/cli.cpp
OBJ      = $(SRC:.cpp=.o)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: clean
