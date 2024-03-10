CXX = g++
CXXFLAGS = -Wall -Wextra -pedantic -std=c++11 -I./src/includes
LDFLAGS = 

SRC_DIR = ./src
BIN_DIR = ./bin
OBJ_DIR = $(BIN_DIR)/obj

SOURCES := $(wildcard $(SRC_DIR)/classes/*.cpp)
OBJECTS := $(SOURCES:$(SRC_DIR)/classes/%.cpp=$(OBJ_DIR)/%.o)
EXECUTABLE := $(BIN_DIR)/main

# Debugging options
ifdef DEBUG
	CXXFLAGS += -g -O0
endif

.PHONY: all clean

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJ_DIR) $(OBJECTS) $(SRC_DIR)/main.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(SRC_DIR)/main.cpp $(OBJECTS) -o $(EXECUTABLE)

$(OBJ_DIR)/%.o: $(SRC_DIR)/classes/%.cpp $(SRC_DIR)/includes/%.h | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(BIN_DIR)

# Debugging target
debug: CXXFLAGS += -DDEBUG
debug: all