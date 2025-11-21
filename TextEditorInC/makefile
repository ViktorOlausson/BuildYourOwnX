CC		:= gcc
CFLAGS  := -Wall -Wextra -std=c99 -g -I. -Iinclude
TEST_CFLAGS := $(CFLAGS) -DTEST_BUILD

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

TEST_OBJS   := tests/test_editor.o src/main_test.o

.PHONY: main test clean

main: $(EDITOR_BIN)

$(EDITOR_BIN): $(EDITOR_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

test: $(TEST_BIN)

$(TEST_BIN): $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $^


src/main_test.o: src/main.c
	$(CC) $(TEST_CFLAGS) -c -o $@ $<


clean:
	rm -f $(EDITOR_BIN) $(TEST_BIN) $(EDITOR_OBJS) $(TEST_OBJS)

fresh: clean main