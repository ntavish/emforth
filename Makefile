LIBS =
CFLAGS = -std=c99 -ggdb -O0 -Wall -Wextra -Wcast-align

SRC=main.c emforth.c interpreter.c builtins.c
HEADERS=$(wildcard *.h)
OBJ=$(SRC:.c=.o)
BUILD_DIR=build
OBJ_FILES=$(addprefix $(BUILD_DIR)/,$(OBJ))
BINARY=$(BUILD_DIR)/emforth
EMSCRIPTEM_BIN=$(BUILD_DIR)/emforth.js
EMCC_SRC=$(SRC) platform_web.c
EMCC_FLAGS= -s USE_PTHREADS=0 \
    -s ASYNCIFY=1 \
    -s WASM=1 \
    -s EXPORT_NAME="EmforthModule" \
    -s MODULARIZE=1 \
    -s NO_EXIT_RUNTIME=1 \
    -s "EXTRA_EXPORTED_RUNTIME_METHODS=['ccall']" \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s ASSERTIONS=1

all: $(BINARY)

web: $(EMSCRIPTEM_BIN)
	# note, i got incorrect syntax in emforth.js, had to manuall fix a "== =" to "===", might get same again
	cp template.html $(BUILD_DIR)/

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BINARY): $(OBJ_FILES) | $(BUILD_DIR)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

$(BUILD_DIR)/%.o: %.c  $(HEADERS) | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(EMSCRIPTEM_BIN): $(EMCC_SRC) | $(BUILD_DIR)
	emcc $^ -o $@ $(EMCC_FLAGS)

clean:
	rm -rf $(BUILD_DIR)

strip: $(BINARY)
	strip --strip-unneeded $^

cloc:
	cloc  --exclude-dir=reference,.vscode  .

format:
	clang-format -i -- **.c **.h

.PHONY: clean cloc strip format
