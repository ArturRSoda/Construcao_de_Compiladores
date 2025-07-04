# Compilador e flags
CXX = g++
CXXFLAGS = -g -std=c++11 -Wall -Isrc/include -Wfatal-errors

# Diretórios
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
OUT_DIR = outputs

# Arquivos fonte e objeto
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

# Nome do executável
TARGET = $(BIN_DIR)/main

# Regra padrão
all: $(TARGET)

# Compila o executável
$(TARGET): $(OBJS)
	@mkdir -p $(OUT_DIR)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Compila cada .cpp para .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpeza
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(OUT_DIR)

.PHONY: all clean

