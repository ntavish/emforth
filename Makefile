LIBS =
CFLAGS = -std=c99 -ggdb -O0 -Wall -Wextra -Wcast-align

SRC=$(wildcard *.c)
HEADERS=$(wildcard *.h)
OBJ=$(SRC:.c=.o)

vm: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

$(OBJ): $(HEADERS) $(SRC)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o vm

cloc:
	cloc  --exclude-dir=reference,.vscode  .
