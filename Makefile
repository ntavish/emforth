LIBS =
CFLAGS = -std=c99 -ggdb -O0 -Wall -Wextra -Wcast-align

# SRC=$(wildcard *.c)
SRC=main.c vm.c vm_interpreter.c builtins.c
HEADERS=$(wildcard *.h)
OBJ=$(SRC:.c=.o)
BINARY=emforth

$(BINARY): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

$(OBJ): $(HEADERS)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o emforth

strip: $(BINARY)
	strip --strip-unneeded $^

cloc:
	cloc  --exclude-dir=reference,.vscode  .

.PHONY: clean cloc strip
