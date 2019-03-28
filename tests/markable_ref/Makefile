BIN := markable_ref_example.run
BUILD_DIR := build
CFLAGS := -std=c++14 -Werror -Wall -Wextra
INCLUDE_DIRS := ../../markable_ref
INCLUDES := $(patsubst %, -I%, $(INCLUDE_DIRS))

all: | build_dir
	g++ $(CFLAGS) $(INCLUDES) -c main.cpp -o $(BUILD_DIR)/main.o
	g++ -o $(BIN) $(BUILD_DIR)/main.o

build_dir:
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(BIN)
