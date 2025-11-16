CC		:= gcc
CFLAGS  := -Wall -Wextra -std=c99 -g -I. -Iinclude

EDITOR_BIN	:= text_editor
TEST_BIN	:= test_editor

.DEFAULT_GOAL := fresh

EDITOR_SRCS := \
    src/main.c \
	#src/editor_rows.c \
    #src/editor_input.c \
    #src/editor_render.c \
    #src/editor_search.c

TEST_SRCS := \
    tests/test_editor.c \
	src/main.c \


EDITOR_OBJS := $(EDITOR_SRCS:.c=.o)
TEST_OBJS   := $(TEST_SRCS:.c=.o)

.PHONY: all test clean

all: $(EDITOR_BIN)
$(EDITOR_BIN): $(EDITOR_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_BIN): $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# src/main.o: src/main.c
# 	$(CC) $(CFLAGS) -c $< -o $@

# tests/test_editor.o: tests/test_editor.c
# 	$(CC) $(CFLAGS) -c $< -o $@

# test: $(TEST_BIN)
# 	./$(TEST_BIN)

clean:
	rm -f $(EDITOR_BIN) $(TEST_BIN) $(EDITOR_OBJS) $(TEST_OBJS)

	
fresh: clean all