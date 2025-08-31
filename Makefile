CXX      := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O0 -g -fsanitize=address -fno-omit-frame-pointer
INCLUDES := -I./
BUILD    := build

OBJS_COMMON := $(BUILD)/common/util.o
OBJS_ENGINE := $(BUILD)/engine/order_book.o $(BUILD)/engine/matching_engine.o

BIN_CLI   := $(BUILD)/tradesim_cli
BIN_TEST  := $(BUILD)/smoke_test

all: $(BIN_CLI) $(BIN_TEST)

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_CLI): $(OBJS_COMMON) $(OBJS_ENGINE) $(BUILD)/cli/tradesim_cli.o
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_TEST): $(OBJS_COMMON) $(OBJS_ENGINE) $(BUILD)/tests/smoke_test.o
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

format:
	clang-format -i common/*.hpp common/*.cpp engine/*.hpp engine/*.cpp cli/*.cpp tests/*.cpp || true

clean:
	rm -rf $(BUILD)



