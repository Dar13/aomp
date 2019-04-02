BIN := lazy_list_example.run
BENCH_BIN := lazy_list_bench.run

BUILD_DIR := build

CFLAGS := -std=c++14 -Wall -Wextra
BENCH_CFLAGS := $(CFLAGS) -Os

INCLUDE_DIRS := ../../lazy_list ../../markable_ref
INCLUDES := $(patsubst %, -I%, $(INCLUDE_DIRS))

all: | build_dir
	g++ $(CFLAGS) $(INCLUDES) -c main.cpp -o $(BUILD_DIR)/main.o
	g++ -o $(BIN) $(BUILD_DIR)/main.o -lpthread
	g++ $(BENCH_CFLAGS) $(INCLUDES) -c bench.cpp -o $(BUILD_DIR)/bench.o
	g++ -o $(BENCH_BIN) $(BUILD_DIR)/bench.o -lpthread

build_dir:
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(BIN)
