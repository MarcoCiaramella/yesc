CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -D_GNU_SOURCE
LDFLAGS = -pthread -lm

# Directories
YESPOWER_DIR = yespower-main
SRC_DIR = .
OBJ_DIR = obj

# Source files
MINER_SOURCES = main.c stratum_client.c json_parser.c
YESPOWER_SOURCES = $(YESPOWER_DIR)/yespower-opt.c $(YESPOWER_DIR)/sha256.c

# Object files
MINER_OBJECTS = $(MINER_SOURCES:%.c=$(OBJ_DIR)/%.o)
YESPOWER_OBJECTS = $(YESPOWER_SOURCES:%.c=$(OBJ_DIR)/%.o)

# Include directories
INCLUDES = -I$(YESPOWER_DIR) -I.

# Targets
TARGET = yespower-miner
TEST_TARGET = $(YESPOWER_DIR)/tests

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJ_DIR) $(MINER_OBJECTS) $(YESPOWER_OBJECTS)
	$(CC) $(MINER_OBJECTS) $(YESPOWER_OBJECTS) -o $@ $(LDFLAGS)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)/$(YESPOWER_DIR)

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/$(YESPOWER_DIR)/%.o: $(YESPOWER_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

test: $(TEST_TARGET)
	cd $(YESPOWER_DIR) && ./tests

$(TEST_TARGET):
	cd $(YESPOWER_DIR) && $(CC) $(CFLAGS) tests.c yespower-ref.c sha256.c -o tests $(LDFLAGS)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
	cd $(YESPOWER_DIR) && rm -f tests

install-deps:
	# Install development packages if needed
	@echo "Make sure you have build-essential installed:"
	@echo "sudo apt-get install build-essential"
