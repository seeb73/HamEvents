CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2 -I./include -I/usr/include/mariadb -I/usr/include/libxml2
LDFLAGS = -pthread -lmysqlclient -lxml2

SRC_DIR = .
OBJ_DIR = build
TARGET = hamevent

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

all: $(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

