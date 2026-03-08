XX      = g++
CXXFLAGS = -O2 -std=c++17 -fopenmp
INCLUDES = -I../.. \
           -I../../database_management \
           -I../../stocastic_simulation

TEST_SRC = enginetest.cpp \
           ../../stocastic_simulation/stocastic_simulation.cpp

TEST_BIN = enginetest

.PHONY: all test clean

all: $(TEST_BIN)

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

clean:
	rm -f $(TEST_BIN)