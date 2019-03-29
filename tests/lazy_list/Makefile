BIN := lazy_list_example.run
BUILD_DIR := build
CFLAGS := -std=c++14 -Wall -Wextra 
INCLUDE_DIRS := ../../lazy_list ../../markable_ref
INCLUDES := $(patsubst %, -I%, $(INCLUDE_DIRS))

all: | build_dir
	g++ $(CFLAGS) $(INCLUDES) -c main.cpp -o $(BUILD_DIR)/main.o
	g++ -o $(BIN) $(BUILD_DIR)/main.o -lpthread

build_dir:
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(BIN)
