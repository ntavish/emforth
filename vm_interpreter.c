/**
 * @file vm_interpreter.c
 *
 * @brief Forth outer, inner interpreter and related functions.
 */

#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "vm_interpreter.h"
#include "vm.h"
#include "builtins_common.h"

/* Forward declarations */
static int read_token(struct forth_ctx *ctx, char *token, size_t max_len);
static int parse_number(char *token, int token_len, stack_cell_t *number_p);

/* this is the inner interpreter, and it is a trampoline */
static void vm_execute(struct forth_ctx *ctx, word_t *start_ip)
{
    ctx->ip = start_ip;

    /* Check if this is a primitive or a colon definition */
    word_t first_word = *start_ip;
    if (first_word != do_docol) {
        /* This is a primitive - execute it once and return */
        ctx->w = start_ip;
        first_word(ctx);
        return;
    }

    /* This is a colon definition - run the inner interpreter */
    while (ctx->ip != NULL) {
        ctx->w = ctx->ip;
        word_t xt = *ctx->ip++;
        if (xt == NULL) {
            break;
        }
        xt(ctx);
    }
}

/**
 * @brief This is what is usually called the outer interpterer
 *
 * What it does:
 * 1. Read a token from input
 * 2. Try to find it in the dictionary
 * 3. If found: execute (immediate mode) or compile (compile mode)
 * 4. If not found: try to parse as number
 * 5. If number: push (immediate mode) or compile literal (compile mode)
 * 6. Otherwise: error
 */
int vm_interpreter(struct forth_ctx *ctx)
{
	char token[MAX_INPUT_LEN + 1];
	int token_len;

	while (1) {
		token_len = read_token(ctx, token, MAX_INPUT_LEN);

		if (token_len < 0) {
			ctx->plat.puts("Error or EOF. Exiting.\n");
			return -1;
		}

		if (token_len == 0) {
			continue;
		}

		/* Try to find word in dictionary */
		dict_header_t *header = find_word_header(ctx, token, token_len);

		if (header) {
			/* Calculate codeword pointer from header */
			char *word_name = (char *)(header + 1);
			word_t *word = (word_t *)((unsigned char *)word_name + ALIGN_UP_WORD_T(header->flags.f.length));

			if (ctx->intrp_data.mode == MODE_IMMEDIATE || header->flags.f.immediate) {
				/* Execute the word */
				vm_execute(ctx, word);
			} else {
				/* Compile mode - compile the word's address */
				word_t first_word = *word;
				if (first_word == do_docol) {
					/* Colon definition - compile the address */
					compile_word(ctx, (stack_cell_t)word);
				} else {
					/* Primitive - compile the function pointer directly */
					compile_word(ctx, (stack_cell_t)first_word);
				}
			}
		} else {
			/* try to parse as number */
			stack_cell_t number;
			if (parse_number(token, token_len, &number) == 0) {
				if (ctx->intrp_data.mode == MODE_IMMEDIATE) {
					/* push number to stack */
					stack_push(ctx, number);
				} else {
					/* compile literal */
					compile_word(ctx, (stack_cell_t)do_lit);
					compile_word(ctx, number);
				}
			}
		}
	}
}

/**
 * parsing decimal and hexadecimal is supported
 */
static int parse_number(char *token, int token_len, stack_cell_t *number_p)
{
	long number;

	if (token_len > 2 && token[0] == '0' && (token[1] == 'x' || token[1] == 'X')) {
		for (int i = 2; i < token_len; i++) {
			if (!isxdigit(token[i])) {
				return -1;
			}
		}
		number = strtol(token, NULL, 16);
	} else {
		for (int i = 0; i < token_len; i++) {
			if (!isdigit(token[i])) {
				return -1;
			}
		}
		number = strtol(token, NULL, 10);
	}

	*number_p = number;

	return 0;
}

void do_docol(struct forth_ctx *ctx)
{
	/* Save return address TODO bounds check */
	/* For top-level execution (rsp==0), save NULL to stop execution after word */
	if (ctx->rsp == 0) {
		ctx->rstack[ctx->rsp++] = NULL;
	} else {
		ctx->rstack[ctx->rsp++] = ctx->ip;
	}
	/* Set IP to body of word (after the codeword) */
	ctx->ip = ctx->w + 1;
}

void do_exit(struct forth_ctx *ctx)
{
	/* Pop IP from return stack */
	if (ctx->rsp > 0) {
		ctx->ip = ctx->rstack[--ctx->rsp];
	} else{
		/* rstack underflow, we have finished execution */
		ctx->ip = NULL;
		return;
	}
}

void do_lit(struct forth_ctx *ctx)
{
	stack_cell_t number = *(stack_cell_t *)(ctx->ip);
	stack_push(ctx, number);
	/* Skip over the literal value */
	ctx->ip += 1;
}

/**
 * @brief Initialize the outer interpreter
 */
void vm_interpreter_init(struct forth_ctx *ctx)
{
	ctx->intrp_data.mode = MODE_IMMEDIATE;
	ctx->intrp_data.in_comment = false;
}


static int read_token(struct forth_ctx *ctx, char *token, size_t max_len)
{
	int ch;
	size_t pos = 0;

	do {
		ch = ctx->plat.getchar();

		if (ch == EOF) {
			return -1;
		}

		/* Handle backslash comments */
		if (ch == '\\') {
			ctx->intrp_data.in_comment = true;
		} else if (ch == '\n' && ctx->intrp_data.in_comment) {
			ctx->intrp_data.in_comment = false;
		}

	} while (isspace(ch)  || ctx->intrp_data.in_comment);

	while (!isspace(ch) && ch != EOF && pos < max_len) {
		token[pos++] = ch;
		ch = ctx->plat.getchar();
	}

	/* Null-terminate */
	token[pos] = '\0';

	return pos;
}

/**
 * returns the pointer to the first word in the definition.
 */
dict_header_t* find_word_header(struct forth_ctx *ctx, const char *name, size_t len)
{
	dict_header_t *header = ctx->dict.latest;

	while (header != DICT_NULL) {
		/* we skip hidden words */
		if (header->flags.f.hidden) {
			header = header->link;
			continue;
		}

		if (header->flags.f.length != len) {
			header = header->link;
			continue;
		}

		char *word_name = (char *)(header + 1);
		if (memcmp(word_name, name, len) == 0) {
			/* Found it - return pointer to header */
			return header;
		}

		/* Move to previous word */
	        header = header->link;
	}

	return NULL;
}
