CC      := clang
CFLAGS  := -Wall -Wextra -Werror -std=c23
LDLIBS  := -lm -lncurses -ltinfo

BUILD_DIR   := build
DEBUG_DIR   := $(BUILD_DIR)/debug
RELEASE_DIR := $(BUILD_DIR)/release

BUILD ?= debug

DEBUG_FLAGS   := -g -O0
RELEASE_FLAGS := -O2 -flto

ifeq ($(BUILD),release)
	CFLAGS     += $(RELEASE_FLAGS)
	TARGET_DIR := $(RELEASE_DIR)
else
	CFLAGS     += $(DEBUG_FLAGS)
	TARGET_DIR := $(DEBUG_DIR)
endif

SRC    := lifetime.c
TARGET := $(TARGET_DIR)/lifetime

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC) | $(TARGET_DIR)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDLIBS)

$(TARGET_DIR):
	@mkdir -p $@

clean:
	$(RM) -r $(BUILD_DIR)
