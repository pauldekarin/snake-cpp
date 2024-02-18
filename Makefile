EXEC=snake
CC=clang++
CLIB=-lcurses

$(EXEC):snake.cpp
	$(CC) -std=c++17 -o $@ $^ $(CLIB) && ./$(EXEC)