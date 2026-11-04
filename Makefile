CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11 -mpopcnt
LDFLAGS = -lm

SRCS = src/types.h src/attacks.c src/board.c src/moves.c src/eval.c src/weights.c src/search.c
OBJS = src/attacks.o src/board.o src/moves.o src/eval.o src/weights.o src/search.o

all: chess_engine
chess_engine: $(OBJS) src/main.o
	$(CC) $(CFLAGS) -o chess_engine $(OBJS) src/main.o $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

tests/test_all: tests/test_all.c $(SRCS)
	$(CC) $(CFLAGS) -I src -o tests/test_all tests/test_all.c src/attacks.c src/board.c src/moves.c src/eval.c src/weights.c src/search.c $(LDFLAGS)

test: tests/test_all
	./tests/test_all
	python3 python/test_integration.py

clean:
	rm -f chess_engine tests/test_all src/*.o