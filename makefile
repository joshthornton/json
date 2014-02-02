CC = gcc
CFLAGS =-Wall -Werror --std=gnu99 -g
SRC =src/
BIN =bin/
INCLUDE=
LIBRARIES=

test: $(SRC)test.c $(SRC)json.c
	$(CC) $(CFLAGS) $(INCLUDE) $(SRC)test.c $(SRC)json.c -o $(BIN)test $(LIBRARIES) 

clean:
	rm -rf $(BIN)*
