CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
LIBS = -lraylib -lm -lpthread -ldl -lrt -lX11

sim: main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o sim $(LIBS)

clean:
	rm -f sim