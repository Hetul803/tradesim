CXX      := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O0 -g -fsanitize=address -fno-omit-frame-pointer
INCLUDES := -I./
BUILD    := build

# Object files
OBJS_COMMON := $(BUILD)/common/util.o $(BUILD)/common/logger.o
OBJS_ENGINE := $(BUILD)/engine/matching_engine.o
OBJS_CLI    := $(BUILD)/cli/tradesim_cli.o
OBJS_TEST   := $(BUILD)/tests/smoke_test.o
OBJS_SERVER := $(BUILD)/net/server.o $(BUILD)/net/main_server.o
OBJS_NETCLI := $(BUILD)/net/client.o
OBJS_BOT_RANDOM := $(BUILD)/bots/bot_random.o
OBJS_BOT_MM     := $(BUILD)/bots/bot_mm.o

# Binaries
BIN_CLI        := $(BUILD)/tradesim_cli
BIN_TEST       := $(BUILD)/smoke_test
BIN_SERVER     := $(BUILD)/tradesim_server
BIN_BOT_RANDOM := $(BUILD)/bot_random
BIN_BOT_MM     := $(BUILD)/bot_mm

all: $(BIN_CLI) $(BIN_TEST) $(BIN_SERVER) $(BIN_BOT_RANDOM) $(BIN_BOT_MM)

# Generic rule to compile any .cpp into build/*.o
$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Link rules
$(BIN_CLI): $(OBJS_COMMON) $(OBJS_ENGINE) $(OBJS_CLI)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_TEST): $(OBJS_COMMON) $(OBJS_ENGINE) $(OBJS_TEST)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_SERVER): $(OBJS_COMMON) $(OBJS_ENGINE) $(OBJS_SERVER)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_BOT_RANDOM): $(OBJS_NETCLI) $(OBJS_BOT_RANDOM)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_BOT_MM): $(OBJS_NETCLI) $(OBJS_BOT_MM)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

format:
	clang-format -i common/*.hpp common/*.cpp engine/*.hpp engine/*.cpp cli/*.cpp tests/*.cpp net/*.hpp net/*.cpp bots/*.cpp || true

clean:
	rm -rf $(BUILD)

