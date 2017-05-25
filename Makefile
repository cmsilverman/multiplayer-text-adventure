OBJS = src/main.o src/utils.o src/message.o src/game.o src/map.o src/handler.o src/dispatch.o src/item.o
DEPS = $(OBJS:.o=.d)
CC = gcc -O3
CFLAGS = -g3 -Wall -fpic -std=gnu99 -MMD -MP
BIN = ./game
LDLIBS = -pthread

.PHONY: all clean

all: $(BIN)
	
$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $(LDLIBS) $(OBJS) -o$(BIN)
	
%.d: %.c

clean:
	-rm -f $(OBJS) $(BIN) src/*.d
