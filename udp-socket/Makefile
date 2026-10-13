# Navnet på det færdige program
TARGET_NAME = mesh_test

# Mapper
SRC_DIR = src
INC_DIR = inc
BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin

# Compiler og indstillinger
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -pthread -I$(INC_DIR)

# Find alle .cpp filer i src-mappen, men FJERN gamle/ubrugte filer fra listen
ALL_SRCS = $(wildcard $(SRC_DIR)/*.cpp)
SRCS = $(filter-out $(SRC_DIR)/node_udp.cpp $(SRC_DIR)/json__1_.cpp, $(ALL_SRCS))

# Generer navne på objekt-filer i build-mappen
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Stien til den endelige binære fil
TARGET = $(BIN_DIR)/$(TARGET_NAME)

# Standard rule
all: $(TARGET)

# Linker-trin
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)
	@echo "Færdig! $(TARGET_NAME) er bygget uden node_udp.cpp."

# Kompilerings-trin
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rengøring
clean:
	rm -rf $(BUILD_DIR)

# Info til debugging
info:
	@echo "Filer der inkluderes: $(SRCS)"
	@echo "Filer der blev sorteret fra: $(SRC_DIR)/node_udp.cpp"

.PHONY: all clean info